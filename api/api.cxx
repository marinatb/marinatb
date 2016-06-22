/*
 * The marina testbed API implementation
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

  // accounts ==================================================================
  srv.onPost( "/accounts/addUser", relay("access", "/accounts/addUser") );
  srv.onPost( "/accounts/userInfo", relay("access", "/accounts/userInfo") );
  srv.onPost( "/accounts/addGroup", relay("access", "/accounts/addGroup") );
  srv.onPost( "/accounts/removeUser", relay("access", "/accounts/removeUser") );
  srv.onPost( "/accounts/removeGroup", relay("access", "/accounts/removeUser") );
  srv.onPost( "/accounts/list", relay("access", "/accounts/removeUser") );

  // blueprint =================================================================
  srv.onPost( "/blueprint/save", relay("access", "/blueprint/save") );
  srv.onPost( "/blueprint/check", relay("access", "/blueprint/check") );
  srv.onPost( "/blueprint/get", relay("access", "/blueprint/get") );
  srv.onPost( "/blueprint/delete", relay("access", "/blueprint/delete") );
  srv.onPost( "/blueprint/list", relay("access", "/blueprint/list") );
  
  // materialization ===========================================================
  srv.onPost( "/materialization/construct", 
      relay("access", "/materialization/construct") );
  
  srv.onPost( "/materialization/destruct", 
      relay("access", "/materialization/destruct") );

  srv.onPost( "/materialization/info", 
      relay("access", "/materialization/info") );
  
  srv.onPost( "/materialization/list", 
      relay("access", "/materialization/list") );
  
  srv.onPost( "/materialization/topo", 
      relay("access", "/materialization/topo") );
  
  srv.onPost( "/materialization/status", 
      relay("access", "/materialization/status") );

  srv.run();
}
