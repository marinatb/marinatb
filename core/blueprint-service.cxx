/*
 * The marina testbed API blueprint service implementation
 */

#include "blueprint.hxx"
#include "db.hxx"
#include "core/util.hxx"
#include "core/compilation.hxx"

using std::string;
using std::unique_ptr;
using std::invalid_argument;
using std::out_of_range;
using std::exception;
using wangle::SSLContextConfig;
using namespace marina;

http::Response save(Json);
http::Response check(Json);
http::Response get(Json);
http::Response del(Json);
http::Response list(Json);

unique_ptr<DB> db{nullptr};

int main()
{
  Glog::init("blueprint-service");

  LOG(INFO) << "blueprint service starting";
  
  db.reset(new DB{"postgresql://murphy:muffins@db"});

  SSLContextConfig sslc;
  sslc.setCertificate(
      "/marina/cert.pem",
      "/marina/key.pem",
      "" //no password on cert
  );
  
  HttpsServer srv("0.0.0.0", 443, sslc);
  
  srv.onPost("/save", jsonIn(save));
  srv.onPost("/get", jsonIn(get));
  srv.onPost("/check", jsonIn(check));
  srv.onPost("/delete", jsonIn(del));
  srv.onPost("/list", jsonIn(list));

  srv.run();
}

http::Response save(Json j)
{
  LOG(INFO) << "save request";

  //extract request parameters
  string project, bpid;
  Json source;
  try
  {
    project = j.at("project").get<string>();
    source = j.at("source");
    bpid = source.at("name");
  }
  catch(out_of_range &) { return badRequest("save", j); }

  //try to save the blueprint
  try 
  {
    db->saveBlueprint(project, source);
    
    Json r;
    r["project"] = project;
    r["bpid"] = bpid;
    r["action"] = "saved"; //d.dump

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("save", j, e); }
}

http::Response get(Json j)
{
  LOG(INFO) << "get request";

  //extract request parameters
  string project, bpid;
  try
  {
    project = j.at("project").get<string>();
    bpid = j.at("bpid").get<string>();
  }
  catch(out_of_range &) { return badRequest("get", j); }

  //try to fetch the blueprint
  try
  {
    Blueprint bp = db->fetchBlueprint(project, bpid);
    return http::Response{ http::Status::OK(), bp.json().dump() };
  }
  //let the caller know if the blueprint does not exist
  catch(out_of_range &)
  {
    LOG(WARNING) << "bad get request" << project << ", " << bpid;
    Json r;
    r["error"] = "unknown project & bpid combination";
    r["project"] = project;
    r["bpid"] = bpid;

    return http::Response{ http::Status::BadRequest(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("get", j, e); }
}

http::Response check(Json j)
{
  LOG(INFO) << "check request";

  //extract request parameters
  string project, bpid;
  try
  {
    project = j.at("project").get<string>();
    bpid = j.at("bpid").get<string>();
  }
  catch(out_of_range &) { return badRequest("check", j); }

  try
  {
    Blueprint bp = db->fetchBlueprint(project, bpid);
    //Diagnostics d = check(bp) ...
    Json r;
    r["project"] = project;
    r["bpid"] = bpid;
    r["diagnostics"] = nullptr; //d.dump

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //let the caller know if the blueprint does not exist
  catch(out_of_range &)
  {
    LOG(WARNING) << "bad get request" << project << ", " << bpid;
    Json r;
    r["error"] = "unknown project & bpid combination";
    r["project"] = project;
    r["bpid"] = bpid;

    return http::Response{ http::Status::BadRequest(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("check", j, e); }

}


http::Response del(Json j)
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

  //do the delete
  try
  {
    db->deleteBlueprint(project, bpid);

    Json r;
    r["project"] = project;
    r["bpid"] = bpid;
    r["action"] = "deleted"; //d.dump

    return http::Response{ http::Status::OK(), r.dump() };
  }
  catch(DeleteActiveBlueprintError&)
  {
    LOG(WARNING) << "attempt to delete active blueprint "
      <<  "("+project+","+bpid+")";

    Json r;
    r["error"] = "active blueprint deletion";
    r["project"] = project;
    r["bpid"] = bpid;

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("del", j, e); }
}

http::Response list(Json)
{
  Json j;
  j["id"] = generate_guid();

  return http::Response{ http::Status::OK(), j.dump() };
}

