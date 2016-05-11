#include "http_request.hxx"
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/ssl/SSLContextConfig.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <proxygen/lib/utils/URL.h>

using namespace folly;
using namespace proxygen;
using namespace std;
using namespace std::chrono;
using namespace marina;

HttpRequest::HttpRequest( HTTPMethod mtd, 
                          const string & url,
                          unique_ptr<IOBuf> msg,
                          size_t recv_window,
                          string log_suffix )
  : httpMethod_{mtd},
    url_{URL(url)},
    recv_window_{recv_window},
    msg_{move(msg)}
{ 
  string log_name = "http_client";
  if(!log_suffix.empty()) log_name += "_" + log_suffix;

  static const string cert_path{"/etc/ssl/certs/ca-certificates.crt"};
  static const string next_protos{"h2,h2-14,spdy/3.1,spdy/3,http/1.1"};

  if(!Glog::initialized) 
  {
    google::InitGoogleLogging(log_name.c_str());
    Glog::initialized = true;
  }

  evb_.reset(new EventBase);

  SocketAddress addr{url_.getHost(), url_.getPort(), true};
  LOG(INFO) << "trying to connect to: " << addr;

  timer_ = HHWheelTimer::UniquePtr{
    new HHWheelTimer{
        evb_.get(),
        milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        AsyncTimeout::InternalEnum::NORMAL,
        milliseconds(5000)}
  };
  connector_.reset(new HTTPConnector{this, timer_.get()});

  static const AsyncSocket::OptionMap opts{{{SOL_SOCKET, SO_REUSEADDR}, 1}};

  if(url_.isSecure())
  {
    initSSL(cert_path, next_protos);
    connector_->connectSSL(
        evb_.get(),
        addr,
        getSSLContext(),
        nullptr,
        milliseconds(3000), //timeout
        opts,
        AsyncSocket::anyAddress(),
        getServerName()
    );
  }
  else
  {
    connector_->connect(evb_.get(), addr, milliseconds(3000), opts);
  }

  evb_.get()->loop();

  LOG(INFO) << "starting";
}

HttpRequest::HttpRequest( HTTPMethod mtd, 
                          const string & url,
                          string msg,
                          size_t recv_window,
                          string log_suffix )
  : HttpRequest(
      mtd,
      url,
      IOBuf::copyBuffer(msg),
      recv_window,
      log_suffix)
{}

HttpRequest::HttpRequest( HTTPMethod mtd, 
                          const string & url,
                          size_t recv_window,
                          string log_suffix )
  : HttpRequest(
      mtd,
      url,
      move(unique_ptr<IOBuf>{nullptr}),
      recv_window,
      log_suffix)
{}


HttpRequest::~HttpRequest() { }

// ssl -------------------------------------------------------------------------

void HttpRequest::initSSL(const string & certPath, const string & nextProtos)
{
  sslContext_ = make_shared<SSLContext>();
  sslContext_->setOptions(SSL_OP_NO_COMPRESSION);
  
  SSLContextConfig config;
  sslContext_->ciphers(config.sslCiphers);
  sslContext_->loadTrustedCertificates(certPath.c_str());
  list<string> ns;
  splitTo<string>(',', nextProtos, inserter(ns, ns.begin()));
  sslContext_->setAdvertisedNextProtocols(ns);
}

void HttpRequest::sslHandshakeFollowup(HTTPUpstreamSession *session) noexcept
{
  auto *sslSocket = dynamic_cast<AsyncSSLSocket*>(session->getTransport());

  const unsigned char* nextProto{nullptr};
  unsigned nextProtoLength = 0;
  sslSocket->getSelectedNextProtocol(&nextProto, &nextProtoLength);
  if(nextProto)
  {
    VLOG(1) 
      << "Client selected next protocol " 
      << string((const char*)nextProto, nextProtoLength);
  }
  else
  {
    VLOG(1) << "Client did not select a next protocol";
  }
}

SSLContextPtr HttpRequest::getSSLContext() { return sslContext_; }

// HTTPConnector::Callback -----------------------------------------------------
    
void HttpRequest::connectSuccess(HTTPUpstreamSession *session)
{
  if(url_.isSecure()) sslHandshakeFollowup(session);

  session->setFlowControl(recv_window_, 
                          recv_window_, 
                          recv_window_);

  request_.dumpMessage(3);

  if(!request_.getHeaders().getNumberOfValues(HTTP_HEADER_USER_AGENT))
    request_.getHeaders().add(HTTP_HEADER_USER_AGENT, "proxygen_curl");
  if(!request_.getHeaders().getNumberOfValues(HTTP_HEADER_HOST))
    request_.getHeaders().add(HTTP_HEADER_HOST, url_.getHostAndPort());
  if(!request_.getHeaders().getNumberOfValues(HTTP_HEADER_ACCEPT))
    request_.getHeaders().add("Accept", "*/*");
  
  txn_ = session->newTransaction(this);
  request_.setMethod(httpMethod_);
  request_.setHTTPVersion(1,1);
  request_.setURL(url_.makeRelativeURL());
  request_.setSecure(url_.isSecure());

  txn_->sendHeaders(request_);
  if(msg_.get() != nullptr) txn_->sendBody(move(msg_));
  txn_->sendEOM();

  session->closeWhenIdle();
}

void HttpRequest::connectError(const AsyncSocketException &ex)
{
  LOG(ERROR) 
    << "Couldn't connect to " << url_.getHostAndPort() << ":" << ex.what();
}

// HTTPTransactionHandler ------------------------------------------------------
    
void HttpRequest::setTransaction(HTTPTransaction *) noexcept { }
  
void HttpRequest::detachTransaction() noexcept { }
    
void HttpRequest::onHeadersComplete(unique_ptr<proxygen::HTTPMessage> msg) noexcept
{
  response_.msg = move(msg);
}

void HttpRequest::onBody(unique_ptr<folly::IOBuf> chain) noexcept
{
  if(response_.content) response_.content->prependChain(move(chain));
  else response_.content = move(chain);
}
  
void HttpRequest::onTrailers(unique_ptr<proxygen::HTTPHeaders>) noexcept
{
  LOG(INFO) << "Discarding trailers";
}
    
void HttpRequest::onEOM() noexcept
{
  LOG(INFO) << "Got EOM";
  response_promise_.set_value(move(response_));
}
    
void HttpRequest::onUpgrade(proxygen::UpgradeProtocol) noexcept
{
  LOG(INFO) << "Discarding upgrade protocol";
}
    
void HttpRequest::onError(const proxygen::HTTPException &error) noexcept
{
  LOG(ERROR) << "An error occurred:" << error.what();
}
    
void HttpRequest::onEgressPaused() noexcept { LOG(INFO) << "Egress paused"; }
    
void HttpRequest::onEgressResumed() noexcept { LOG(INFO) << "Egress resumed"; }

// misc ------------------------------------------------------------------------

const string & HttpRequest::getServerName() const
{
  const string &res = request_.getHeaders().getSingleOrEmpty(HTTP_HEADER_HOST);
  if(res.empty()) return url_.getHost();
  return res;
}

// result ----------------------------------------------------------------------
future<http::Message> HttpRequest::response()
{
  return response_promise_.get_future();
}

