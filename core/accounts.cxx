/*
 * The marina testbed API accounts service implementation
 */

#include "common/net/http_server.hxx"

using namespace marina;
using wangle::SSLContextConfig;
using std::string;

static const string not_implemented{R"({"error": "not implemented"})"};

int main()
{
  SSLContextConfig sslc;
  sslc.setCertificate(
      "/marina/cert.pem",
      "/marina/key.pem",
      "" //no password on cert
  );

  HttpsServer srv("0.0.0.0", 443, sslc);

  srv.onPost("/addUser", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });
  
  srv.onPost("/userInfo", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });
  
  srv.onPost("/addGroup", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });

  srv.onPost("/removeUser", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });
  
  srv.onPost("/removeGroup", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });

  srv.onPost("/list", [](http::Message)
  {
    return http::Response{ http::Status::OK(), not_implemented };
  });

  srv.run();
    
}
