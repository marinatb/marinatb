/*------------------------------------------------------------------------------
 * This code performs tests on the blueprint service through the API
 * ---------------------------------------------------------------------------*/

#include "../catch.hpp"
#include "common/net/http_request.hxx"
#include "test/models/blueprints/blueprints.hxx"
#include "core/db.hxx"
#include "test/models/topos/topologies.hxx"

using std::string;
using proxygen::HTTPMethod;
using namespace marina;

static Blueprint m = mars();

Json svcRequest(string svc, string path, Json msg)
{
  HttpRequest rq{
    HTTPMethod::POST,
    "https://api/" +svc + "/" + path,
    msg.dump(2)
  };
  auto res = rq.response().get();
  REQUIRE( res.msg->getStatusCode() == 200 );
  return res.bodyAsJson();
}

TEST_CASE("mars-construct", "[api-mzn]")
{
  DB db{"postgresql://murphy:muffins@db"};
  db.setHwTopo(minibed().json());

  Json rq;
  rq["project"] = "backyard";
  rq["source"] = m.json();
  svcRequest("blueprint", "save", rq);

  rq = Json{};
  rq["project"] = "backyard";
  rq["bpid"] = "mars";
  auto res = svcRequest("materialization", "construct", rq);

  REQUIRE( res.at("project").get<string>() == "backyard" );
  REQUIRE( res.at("bpid").get<string>() == "mars" );
  REQUIRE( res.at("action").get<string>() == "constructed" );

  Json minfo = svcRequest("materialization", "info", rq);
  Blueprint bp = Blueprint::fromJson(minfo);

  for(Computer & c : bp.computers())
  {
    Embedding e = c.embedding();
    REQUIRE( e.host != "goblin" );
    REQUIRE( e.assigned == true );
  }

  res = svcRequest("materialization", "destruct", rq);
  
  REQUIRE( res.at("project").get<string>() == "backyard" );
  REQUIRE( res.at("bpid").get<string>() == "mars" );
  REQUIRE( res.at("action").get<string>() == "deconstructed" );

  res = svcRequest("blueprint", "delete", rq);
  
  REQUIRE( res.at("project").get<string>() == "backyard" );
  REQUIRE( res.at("bpid").get<string>() == "mars" );
  REQUIRE( res.at("action").get<string>() == "deleted" );

}