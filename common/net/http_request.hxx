#ifndef MARINATB_COMMON_NET_HTTP_CLIENT_HXX
#define MARINATB_COMMON_NET_HTTP_CLIENT_HXX

#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/utils/URL.h>
#include <future>
#include <atomic>
#include "proto.hxx"
#include "glog.hxx"

namespace marina
{

  class HttpRequest : public proxygen::HTTPConnector::Callback,
                      public proxygen::HTTPTransactionHandler
  {
    public:

      HttpRequest(
        proxygen::HTTPMethod,
        const std::string & url,
        std::unique_ptr<folly::IOBuf> msg,
        size_t recv_window=65536,
        std::string log_suffix="");
      
      HttpRequest(
        proxygen::HTTPMethod,
        const std::string & url,
        std::string msg,
        size_t recv_window=65536,
        std::string log_suffix="");
      
      HttpRequest(
        proxygen::HTTPMethod,
        const std::string & url,
        size_t recv_window=65536,
        std::string log_suffix="");

      HttpRequest(HttpRequest &&) = default;
      HttpRequest & operator= (HttpRequest &&) = default;

      ~HttpRequest() override;

      // ssl
      void initSSL(const std::string & certPath,
                  const std::string & nextProtos);
      void sslHandshakeFollowup(proxygen::HTTPUpstreamSession*) noexcept;
      folly::SSLContextPtr getSSLContext();

      // HTTPConnector::Callback
      void connectSuccess(proxygen::HTTPUpstreamSession*) override;
      void connectError(const folly::AsyncSocketException&) override;

      // HTTPTransactionHandler
      void setTransaction(proxygen::HTTPTransaction*) noexcept override;
      void detachTransaction() noexcept override;
      void onHeadersComplete(
          std::unique_ptr<proxygen::HTTPMessage>) noexcept override;
      void onBody(std::unique_ptr<folly::IOBuf>) noexcept override;
      void onTrailers(std::unique_ptr<proxygen::HTTPHeaders>) noexcept override;
      void onEOM() noexcept override;
      void onUpgrade(proxygen::UpgradeProtocol) noexcept override;
      void onError(const proxygen::HTTPException&) noexcept override;
      void onEgressPaused() noexcept override;
      void onEgressResumed() noexcept override;

      // misc
      const std::string & getServerName() const;

      // result
      std::future<http::Message> response();

    protected:
      proxygen::HTTPTransaction *txn_{nullptr};
      std::unique_ptr<folly::EventBase> evb_{nullptr};
      proxygen::HTTPMethod httpMethod_;
      proxygen::URL url_;
      proxygen::HTTPMessage request_;
      folly::SSLContextPtr sslContext_;
      http::Message response_;

      size_t recv_window_;
      std::unique_ptr<folly::IOBuf> msg_;
      std::unique_ptr<proxygen::HTTPConnector> connector_{nullptr};
      folly::HHWheelTimer::UniquePtr timer_{nullptr};

      std::promise<http::Message> response_promise_{};
  };

}

#endif
