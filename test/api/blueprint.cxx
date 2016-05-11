/*------------------------------------------------------------------------------
 * This code performs tests on the blueprint service through the API
 * ---------------------------------------------------------------------------*/

#include "../catch.hpp"
#include "common/net/http_request.hxx"
#include "test/models/blueprints/blueprints.hxx"

using std::string;
using std::vector;
using namespace marina;
using proxygen::HTTPMethod;
  
static Blueprint m = mars();

Json blueprintRequest(string path, Json msg)
{
  HttpRequest rq{
    HTTPMethod::POST,
    "https://api/blueprint/" + path,
    msg.dump(2)
  };
  auto res = rq.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  return res.bodyAsJson();
}

TEST_CASE("mars-save", "[api-blueprint]")
{
  Json j;
  j["project"] = "backyard";
  j["source"] = m.json();

  Json reply = blueprintRequest("save", j);
  REQUIRE( reply.at("project") == "backyard" );
  REQUIRE( reply.at("bpid") == "mars" );
  REQUIRE( reply.at("action") == "saved" );
}

TEST_CASE("mars-get", "[api-blueprint]")
{
  Json msg;
  msg["project"] = "backyard";
  msg["bpid"] = "mars";

  Json reply = blueprintRequest("get", msg);
  REQUIRE( reply.at("name") == "mars" );
}

TEST_CASE("mars-update", "[api-blueprint]")
{
  Json g;
  g["project"] = "backyard";
  g["bpid"] = "mars";

  Json bp = blueprintRequest("get", g);
  vector<Json> nets = bp.at("networks");
  REQUIRE(nets.size() == 3);

  m.network("tignish")
    .capacity(47_gbps)
    .latency(5_ms);
  
  Json s;
  s["project"] = "backyard";
  s["source"] = m.json();

  Json reply = blueprintRequest("save", s);

  bp = blueprintRequest("get", g);
  nets = bp.at("networks").get<vector<Json>>();
  REQUIRE( nets.size() == 4 );
}

TEST_CASE("mars-check", "[api-blueprint]")
{
  Json g;
  g["project"] = "backyard";
  g["bpid"] = "mars";

  Json reply = blueprintRequest("check", g);
  REQUIRE( reply.at("project").get<string>() == "backyard" );
  REQUIRE( reply.at("bpid").get<string>() == "mars" );
  REQUIRE( reply.at("diagnostics") == nullptr );
}


TEST_CASE("mars-del", "[api-blueprint]")
{
  Json g;
  g["project"] = "backyard";
  g["bpid"] = "mars";

  Json reply = blueprintRequest("delete", g);
  REQUIRE( reply.at("project").get<string>() == "backyard" );
  REQUIRE( reply.at("bpid").get<string>() == "mars" );
  REQUIRE( reply.at("action").get<string>() == "deleted" );
}

/*
TEST_CASE("mars-list", "[api-blueprint]")
{
  Json msg;

  Json reply = blueprintRequest("list", msg);
  string id_ = reply.at("id");
  REQUIRE( id.size() == 36 );
}
*/

