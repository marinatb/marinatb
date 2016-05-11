#include "common/net/http_server.hxx"
#include "common/net/http_request.hxx"
#include <proxygen/httpserver/HTTPServer.h>
#include "../../catch.hpp"

using std::thread;
using std::move;

using Protocol = proxygen::HTTPServer::Protocol;
using wangle::SSLContextConfig;
using namespace marina;

TEST_CASE("http-server-basic-test", "[net]")
{
  SSLContextConfig sslc;
  sslc.setCertificate(
      "/etc/letsencrypt/live/marina.deterlab.net/cert.pem",
      "/etc/letsencrypt/live/marina.deterlab.net/privkey.pem",
      "" //no password on cert
  );

  HttpsServer srv("localhost", 4433, sslc);
  
  srv.onGet("/", [](http::Message) {
    return http::Response{
      http::Status::OK(),
      "well, hello there!"
    };
  });

  srv.onGet("/relay", [](http::Message m) {

      HttpRequest req{
        m.msg->getMethod().get(),
        "https://marina.deterlab.net",
        move(m.content)
      };
        
      auto r = req.response().get();

      return http::Response {
        http::Status{r.msg->getStatusCode(), r.msg->getStatusMessage()},
        move(r.content)
      };

  });

  srv.run();
}
