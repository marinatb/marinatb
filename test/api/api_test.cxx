#include "../catch.hpp"
#include "common/net/http_request.hxx"

using namespace marina;
using proxygen::HTTPMethod;
using std::string;

static const string not_implemented{R"({"error": "not implemented"})"};

//accounts =====================================================================

TEST_CASE("marina-api-basic", "[api]")
{
  // addUser -------------------------------------------------------------------
  HttpRequest req{
    HTTPMethod::POST,
    "https://api/accounts/addUser",
    R"({"id-token": "a4be87909fea896889d8978c"})"
  };
  auto res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // userInfo ------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/accounts/userInfo",
    R"({"uid": "a4be87909fea896889d8978c"})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // addGroup ------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/accounts/addGroup",
    R"({"gid": "a4be87909fea896889d8978c"})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // removeUser ----------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/accounts/removeUser",
    R"({"uid": "a4be87909fea896889d8978c"})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // removeGroup ---------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/accounts/removeGroup",
    R"({"gid": "a4be87909fea896889d8978c"})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // removeGroup ---------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/accounts/list",
    R"({})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );

}

//blueprint ====================================================================
TEST_CASE("marina-api-blueprint", "[api]")
{
  // save ----------------------------------------------------------------------
  HttpRequest req{
    HTTPMethod::POST,
    "https://api/blueprint/save",
    R"({"blueprint": {}})"
  };
  auto res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  //REQUIRE( res.bodyAsString() == not_implemented );
  
  // check ---------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/blueprint/check",
    R"({"id": {}})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  //REQUIRE( res.bodyAsString() == not_implemented );
  
  // get -----------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/blueprint/get",
    R"({"id": {}})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  //REQUIRE( res.bodyAsString() == not_implemented );
  
  // delete --------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/blueprint/delete",
    R"({"id": {}})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  //REQUIRE( res.bodyAsString() == not_implemented );
  
  // list ----------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/blueprint/list",
    R"({})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  //REQUIRE( res.bodyAsString() == not_implemented );
}

//materialization ==============================================================
TEST_CASE("marina-api-materialization", "[api]")
{
  // construct -----------------------------------------------------------------
  HttpRequest req{
    HTTPMethod::POST,
    "https://api/materialization/construct",
    R"({"id": {}})"
  };
  auto res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // destruct ------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/materialization/destruct",
    R"({"id": {}})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // info ----------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/materialization/info",
    R"({"id": {}})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );
  
  // list ----------------------------------------------------------------------
  req = HttpRequest{
    HTTPMethod::POST,
    "https://api/materialization/list",
    R"({})"
  };
  res = req.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  REQUIRE( res.bodyAsString() == not_implemented );

}
