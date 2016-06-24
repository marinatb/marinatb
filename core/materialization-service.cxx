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
using std::unordered_set;
using std::future;
using std::vector;
using std::exception;
using std::out_of_range;
using std::endl;
using wangle::SSLContextConfig;
using proxygen::HTTPMethod;
using namespace marina;

static const string not_implemented{R"({"error": "not implemented"})"};

http::Response construct(Json);
http::Response destruct(Json);
http::Response info(Json);
http::Response list(Json);
http::Response topo(Json);
http::Response status(Json);

unique_ptr<DB> db{nullptr};

int main(int argc, char **argv)
{
  Glog::init("mzn-service");
  LOG(INFO) << "materialization service starting";

  gflags::SetUsageMessage("usage: materialization");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

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
  srv.onPost("/topo", jsonIn(topo));
  srv.onPost("/status", jsonIn(status));

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
  catch(out_of_range &e) { return badRequest("save", j, e); }

  //try to perform the materialization
  try
  {
    // get requested blueprint and hw topology from database
    Blueprint bp = db->fetchBlueprint(project, bpid);
    TestbedTopology topo = db->fetchHwTopo();
    EChart ec{topo};

    // compute the materialization embedding
    // --
    // this will fill in bp with the materialization details and return a clone 
    // of topo that bas bp emedded inside

    // TODO: xxx
    auto embedding = embed(bp, ec, topo);
    
    //TODO vxlan.vni: this is a centralized database attribute for now
    //however in the future this should be a distributed agreement variable
    //among the host-controllers because at this level we really shouldn't
    //care about such a materialization implementation detail. Maybehapps
    //this could be a place to use riak/redis/memcached @ the host-controller 
    //level
    for(auto & p : bp.networks())
    {
      Network & n = p.second;
      //setup vxlan virtual network identifier
      n.einfo().vni = db->newVxlanVni(n.id().str());

      //set interface ip addresses
      IpV4Address a = n.ipv4();

      for( auto & c : bp.connectedComputers(n) )
      {
        if(a.netZero()) a++;
        Interface & ifx = c.second;
        ifx.einfo().ipaddr_v4 = a;
        a++;
      }
    }

    //call out to all of the selected materialization hosts asking them to 
    //materialize their portion of the blueprint
    vector<future<http::Message>> replys;

    //TODO: need to get a subset of hosts specific to this blueprint
    //as it is now all hosts will get embedding commands
    for(const auto & h : embedding.hmap)
    {
      //TODO with new embedding code
      /*
      Blueprint lbp = bp.localEmbedding(h.host.name());

      replys.push_back(
        HttpRequest
        {
          HTTPMethod::POST,
          "https://"+h.host.name()+"/construct",
          //rq.dump()
          lbp.json().dump()
        }
        .response()
      );
      */
    }

    // save the embedding to the database
    db->saveMaterialization(project, bpid, bp.json());
    //db->setHwTopo(embedding.json());

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
  catch(out_of_range &e) { return badRequest("save", j, e); }

  //get the materialization info
  try
  {
    Blueprint mzn = db->fetchMaterialization(project, bpid);
    return http::Response{ http::Status::OK(), mzn.json().dump() };
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
  catch(out_of_range &e) { return badRequest("del", j, e); }

  try
  {
    Blueprint bp = db->fetchMaterialization(project, bpid);
    TestbedTopology topo = db->fetchHwTopo();

    EChart ec{topo}; // = db->fetchEChart();
    
    //compute the set of hosts containing computers in this blueprint
    unordered_set<string> hosts;
    for(const auto & c : bp.computers()) 
    {
      for(const auto & h : ec.hmap)
      {
        if(h.machines.find(c.second.id()) != h.machines.end()) 
          hosts.insert(h.host.name());
      }
    }
    /*
    for(const auto & c : bp.computers()) 
    {
      hosts.insert(c.second.embedding().host);
    }
    */

    for(const auto & n : bp.networks())
    {
      db->freeVxlanVni(n.second.id().str());
    }
   
    //async command to remove computers and networks from relevant hosts
    vector<future<http::Message>> replys;
    for(const string & h : hosts)
    {
      LOG(INFO) << "destructing " << bp.name() << " on " << h;
      //TODO with new embedding structure
      /*
      Blueprint lbp = bp.localEmbedding(h);  
      replys.push_back(
        HttpRequest
        {
          HTTPMethod::POST,
          "https://"+h+"/destruct",
          lbp.json().dump()
        }
        .response()
      );
      */
    }
    
    //TODO: xxx
    //TestbedTopology topo_ = unembed(bp, topo);
    TestbedTopology topo_ = topo;
    
    db->deleteMaterialization(project, bpid);
    db->setHwTopo(topo_.json());

    Json r;
    r["project"] = project;
    r["bpid"] = bpid;
    r["action"] = "deconstructed"; //d.dump

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("del", j, e); }
}

http::Response list(Json j)
{
  LOG(INFO) << "list request";

  //extract request parameters
  string project;
  try
  {
    project = j.at("project");
  }
  catch(out_of_range &e) { return badRequest("list", j, e); }

  try
  {
    auto bps = db->fetchMaterializations(project);

    Json r;
    r["status"] = "ok";
    r["materializations"] = jtransform(bps);

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("list", j, e); }
}

http::Response topo(Json j)
{
  LOG(INFO) << "topo request";

  try
  {
    TestbedTopology t = TestbedTopology::fromJson(j);
    db->setHwTopo(t.json());

    Json r;
    r["status"] = "ok";
    r["action"] = "topo";

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("topo", j, e); }

}

http::Response status(Json j)
{
  LOG(INFO) << "status request";

  try
  {
    TestbedTopology t = db->fetchHwTopo();

    Json r;
    r["status"] = "ok";
    r["topo"] = t.json();

    return http::Response{ http::Status::OK(), r.dump() };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("topo", j, e); }
}

