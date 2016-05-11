#ifndef PROXYGEN_PLAY_RYSRV_HXX
#define PROXYGEN_PLAY_RYSRV_HXX

#include <folly/Memory.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <functional>
#include "proto.hxx"
#include "glog.hxx"
#include "http_request.hxx"

namespace marina
{
  class SrvStats
  {
    public:
      void recordRequest() { ++cnt_; }
      size_t getRequestCount() { return cnt_; }
    private:
      size_t cnt_{0};
  };
  
  struct Handler
  {
    std::function<http::Response(http::Message)> f;
    proxygen::HTTPMethod method;
    std::string path;
  };

  class RqHandler : public proxygen::RequestHandler
  {
    public:
      RqHandler(std::vector<Handler> handlers, SrvStats*);

      //RequestHandler
      void onRequest(std::unique_ptr<proxygen::HTTPMessage>) noexcept override;
      void onBody(std::unique_ptr<folly::IOBuf>) noexcept override;
      void onEOM() noexcept override;
      void onUpgrade(proxygen::UpgradeProtocol) noexcept override;
      void requestComplete() noexcept override;
      void onError(proxygen::ProxygenError) noexcept override;

    private:
      SrvStats *stats_{nullptr};
      std::unique_ptr<folly::IOBuf> body_{nullptr};
      std::unique_ptr<proxygen::HTTPMessage> msg_{nullptr};
      std::vector<Handler> handlers_;
  };
  

  class RqHandlerFactory : public proxygen::RequestHandlerFactory
  {
    public:
      RqHandlerFactory(std::vector<Handler> handlers)
        : handlers_{handlers}
      { }

      void onServerStart(folly::EventBase *) noexcept override 
      { 
        stats_.reset(new SrvStats); 
      }
      void onServerStop() noexcept override { stats_.reset(); }

      proxygen::RequestHandler* 
      onRequest(proxygen::RequestHandler*, proxygen::HTTPMessage*) 
      noexcept override
      {
        return new RqHandler{handlers_, stats_.get()};
      }

    private:
      folly::ThreadLocalPtr<SrvStats> stats_;
      std::vector<Handler> handlers_;
  };

  class HttpsServer
  {
    public:
      HttpsServer(std::string addr, size_t port, wangle::SSLContextConfig sslc);

      void onGet(std::string url, std::function<http::Response(http::Message)>);
      void onPost(std::string url, std::function<http::Response(http::Message)>);

      void run();

    private:
      std::string addr_;
      size_t port_;
      wangle::SSLContextConfig sslc_;
      proxygen::HTTPServerOptions srv_opts_;
      std::unique_ptr<proxygen::HTTPServer> server_{nullptr};
      std::vector<Handler> handlers_;
      std::vector<proxygen::HTTPServer::IPConfig> ips_;
  };

  //helpful misc functions
  std::function<http::Response(http::Message)> 
  relay(std::string host, std::string path);

}

#endif
