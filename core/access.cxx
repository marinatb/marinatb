/*
 * The marina testbed API access-control service implementation
 */

#include "common/net/http_server.hxx"
#include "common/net/http_request.hxx"

using namespace marina;
using wangle::SSLContextConfig;
using std::string;
using std::move;

int main()
{
  SSLContextConfig sslc;
  sslc.setCertificate(
      "/marina/cert.pem",
      "/marina/key.pem",
      "" //no password on cert
  );

  HttpsServer srv("0.0.0.0", 443, sslc);

  //TODO actually check access

  // accounts ==================================================================
  srv.onPost( "/accounts/addUser", relay("accounts", "/addUser") );
  srv.onPost( "/accounts/userInfo", relay("accounts", "/userInfo") );
  srv.onPost( "/accounts/addGroup", relay("accounts", "/addGroup") );
  srv.onPost( "/accounts/removeUser", relay("accounts", "/removeUser") );
  srv.onPost( "/accounts/removeGroup", relay("accounts", "/removeUser") );
  srv.onPost( "/accounts/list", relay("accounts", "/removeUser") );
  
  // blueprint =================================================================
  srv.onPost( "/blueprint/save", relay("blueprint", "/save") );
  srv.onPost( "/blueprint/check", relay("blueprint", "/check") );
  srv.onPost( "/blueprint/get", relay("blueprint", "/get") );
  srv.onPost( "/blueprint/delete", relay("blueprint", "/delete") );
  srv.onPost( "/blueprint/list", relay("blueprint", "/list") );
  
  // materialization ===========================================================
  srv.onPost( "/materialization/construct", 
      relay("materialization", "/construct") );
  
  srv.onPost( "/materialization/destruct", 
      relay("materialization", "/destruct") );

  srv.onPost( "/materialization/info", 
      relay("materialization", "/info") );
  
  srv.onPost( "/materialization/list", 
      relay("materialization", "/list") );

  srv.run();
    
}
