#include "http_server.hxx"
#include <proxygen/httpserver/ResponseBuilder.h>

using std::string;
using std::to_string;
using std::vector;
using std::unique_ptr;
using std::move;
using std::max;
using std::function;
using std::thread;
using std::find_if;
using namespace std::chrono;

using proxygen::HTTPServerOptions;
using proxygen::HTTPServer;
using proxygen::HTTPMessage;
using proxygen::ResponseBuilder;
using proxygen::RequestHandlerChain;
using proxygen::HTTPMethod;
using Protocol = HTTPServer::Protocol;

using folly::IOBuf;
using folly::SocketAddress;

using wangle::SSLContextConfig;

using namespace marina;

RqHandler::RqHandler(vector<Handler> handlers, SrvStats *stats) 
  : stats_{stats},
    handlers_{handlers}
{
  if(!Glog::initialized)
  {
    google::InitGoogleLogging("marina-http-server");
    google::InstallFailureSignalHandler();
    Glog::initialized = true;
  }
}


//TODO we should reject here if the url does not match one of the
//user specified handlers instead of rx'ing the whole message
void RqHandler::onRequest(unique_ptr<proxygen::HTTPMessage> msg) noexcept
{
  msg_ = move(msg);
  stats_->recordRequest();
}
    
void RqHandler::onBody(unique_ptr<folly::IOBuf> body) noexcept
{
  if(body_) body_->prependChain(move(body));
  else body_ = move(body);
}
    
void RqHandler::onEOM() noexcept
{
  string path = msg_->getPath();
  auto method = msg_->getMethod();

  auto handler_it = find_if(handlers_.begin(), handlers_.end(),
    [&path, &method](const Handler &h)
    {
      return h.path == path && h.method == method;
    } 
  );

  if(handler_it != handlers_.end())
  {
    auto h = *handler_it;
    auto response = h.f( http::Message{move(msg_), move(body_)} );

    ResponseBuilder(downstream_)
      .status(response.status.code, response.status.message)
      .body(move(response.content))
      .sendWithEOM();
  }
  else
  {
    ResponseBuilder(downstream_)
      .status(404, "Not Found")
      .sendWithEOM();
  }
}

void RqHandler::onUpgrade(proxygen::UpgradeProtocol) noexcept {}
void RqHandler::requestComplete() noexcept { delete this; }
void RqHandler::onError(proxygen::ProxygenError) noexcept { delete this; }

HttpsServer::HttpsServer(string ip, size_t http_port, SSLContextConfig sslc)
  : addr_{ip},
    port_{http_port},
    sslc_(sslc)
{
  sslc_.isDefault = true;

  HTTPServer::IPConfig cfg{
    SocketAddress(ip, port_, true), Protocol::HTTP
  };
  cfg.sslConfigs.push_back(sslc_);

  ips_ = { cfg };

  size_t threads = max({4l, sysconf(_SC_NPROCESSORS_ONLN)});

  srv_opts_.threads = static_cast<size_t>(threads);
  srv_opts_.idleTimeout = milliseconds(60000);
  srv_opts_.shutdownOn = {SIGINT, SIGTERM};
  srv_opts_.enableContentCompression = true;
}
 
void HttpsServer::onGet(string url, function<http::Response(http::Message)> f)
{
  handlers_.push_back( {f, HTTPMethod::GET, url} );
}

void HttpsServer::onPost(string url, function<http::Response(http::Message)> f)
{
  handlers_.push_back( {f, HTTPMethod::POST, url} );
}

void HttpsServer::run()
{
  srv_opts_.handlerFactories = 
    RequestHandlerChain{}
      .addThen<RqHandlerFactory>(handlers_)
      .build();

  server_.reset(new HTTPServer(move(srv_opts_)));
  server_->bind(ips_);

  thread t{[this](){ server_->start(); }};
  t.join();
}

function<http::Response(http::Message)> 
marina::relay(string host, string path)
{
  return 
  [host,path](http::Message m)
  {
    HttpRequest req{
      m.msg->getMethod().get(),
      "https://"+host+path,
      move(m.content)
    };

    auto r = req.response().get();

    return http::Response{
      http::Status{r.msg->getStatusCode(), r.msg->getStatusMessage()},
      move(r.content)
    };
  };
}
