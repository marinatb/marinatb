/*
 * The marina testbed API materialization service implementation
 */

#include "common/net/http_server.hxx"
#include "core/embed.hxx"
#include "core/blueprint.hxx"
#include "core/topo.hxx"
#include "core/db.hxx"
#include "core/util.hxx"

using std::string;
using std::unique_ptr;
using std::future;
using std::vector;
using std::exception;
using std::out_of_range;
using wangle::SSLContextConfig;
using proxygen::HTTPMethod;
using namespace marina;

static const string not_implemented{R"({"error": "not implemented"})"};

http::Response construct(Json);
http::Response destruct(Json);
http::Response info(Json);
http::Response list(Json);

unique_ptr<DB> db{nullptr};

int main()
{
  Glog::init("materialization-service");

  LOG(INFO) << "materialization service starting";

  db.reset(new DB{"postgresql://murphy:muffins@db"});

  SSLContextConfig sslc;
  sslc.setCertificate(
      "/marina/cert.pem",
      "/marina/key.pem",
      "" //no password on cert
  );
  
  HttpsServer srv("0.0.0.0", 443, sslc);
  
  srv.onPost("/construct", jsonIn(construct));
  srv.onPost("/destruct", jsonIn(destruct));
  srv.onPost("/info", jsonIn(info));
  srv.onPost("/list", jsonIn(list));

  srv.run();
}

http::Response construct(Json j)
{
  LOG(INFO) << "construct request";
  
  //extract request parameters
  string project, bpid;
  try
  {
    project = j.at("project");
    bpid = j.at("bpid");
  }
  catch(out_of_range &) { return badRequest("save", j); }

  //try to perform the materialization
  try
  {
    // get requested blueprint and hw topology from database
    Blueprint bp = db->fetchBlueprint(project, bpid);
    TestbedTopology topo = db->fetchHwTopo();

    // compute the materialization embedding
    // --
    // this will fill in bp with the materialization details and return a clone 
    // of topo that bas bp emedded inside
    auto embedding = embed(bp, topo);

    //call out to all of the selected materialization hosts asking them to 
    //materialize their portion of the blueprint
    /*
    vector<future<http::Message>> replys;
    for(const Computer & c : bp.computers())
    {
      string host = c.embedding().host;

      replys.push_back(
        HttpRequest
        {
          HTTPMethod::POST,
          "https://"+host+"/materialize",
          c.json().dump()
        }
        .response()
      );
    }
    */

    // save the embedding to the database
    db->saveMaterialization(project, bpid, bp.json());
    db->setHwTopo(embedding.json());

    // return result to caller
    j["action"] = "constructed";
    return http::Response{ http::Status::OK(), j.dump() };
  }
  catch(exception &e) { return unexpectedFailure("construct", j, e); }
}

http::Response info(Json j)
{
  //extract request parameters
  string project, bpid;
  try
  {
    project = j.at("project");
    bpid = j.at("bpid");
  }
  catch(out_of_range &) { return badRequest("save", j); }

  //get the materialization info
  try
  {
    Json mzn = db->fetchMaterialization(project, bpid);
    return http::Response{ http::Status::OK(), mzn.dump() };
  }
  catch(exception &e) { return unexpectedFailure("construct", j, e); }

  return http::Response{ http::Status::OK(), not_implemented };
}

http::Response destruct(Json j)
{
  LOG(INFO) << "del request";

  //extract request parameters
  string project, bpid;
  try
  {
    project = j.at("project").get<string>();
    bpid = j.at("bpid").get<string>();
  }
  catch(out_of_range &) { return badRequest("del", j); }

  try
  {
    db->deleteMaterialization(project, bpid);

    Json r;
    r["project"] = project;
    r["bpid"] = bpid;
    r["action"] = "deconstructed"; //d.dump

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("del", j, e); }
}

http::Response list(Json)
{
  return http::Response{ http::Status::OK(), not_implemented };
}

