//TODO: update stringstream based queries to fmt::format ones

#include <unistd.h>
#include <stdexcept>
#include <sstream>
#include <fmt/format.h>
#include "util.hxx"
#include "core/db.hxx"

using std::string;
using std::vector;
using std::stoul;
using std::runtime_error;
using std::out_of_range;
using std::move;
using std::stringstream;
using namespace marina;

DB::DB(string address)
  : address_{address},
    conn_{PQconnectdb(address.c_str())}
{
  while(PQstatus(conn_) != CONNECTION_OK)
  {
    string msg{"connection to database failed. connection string: " + address};
    LOG(ERROR) << msg;
    usleep(33e4);
    conn_ = PQconnectdb(address.c_str());
  }
}

DB::~DB()
{
  PQfinish(conn_);
}

// blueprint +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

string DB::saveBlueprint(string project, Json src)
{
  string s{"'"+src.dump()+"'"},
         p{"'"+project+"'"};

  stringstream ss;
  ss
    << "INSERT INTO blueprints (project, doc) values("
    <<    "(select id FROM projects WHERE name="+p+"), "
    <<    s << ") "
    << "ON CONFLICT (project, (doc->>'name')) " 
    << "DO UPDATE SET doc = "<< s <<" RETURNING id";

  string q = ss.str();

  PGresult *res = PQexec( conn_, q.c_str() );

  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "save source failed";
    LOG(INFO) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  string id{PQgetvalue(res, 0, 0)};
  LOG(INFO) << "saved blueprint " << id;

  PQclear(res);
  return id;
}

Blueprint DB::fetchBlueprint(string project, string bp_name)
{
  stringstream ss;
  ss << "SELECT doc FROM blueprints where " 
     << "project = " 
        << "(SELECT id FROM projects WHERE name = " 
        << "'" << project << "') AND "
     << "(doc->>'name') = '" << bp_name << "'";

  string q = ss.str();

  PGresult *res = PQexec( conn_, q.c_str() );
  
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "add_blueprint failed";
    LOG(INFO) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  if(PQntuples(res) == 0)
  {
    throw out_of_range{"("+project+", "+bp_name+") not found"};
  }

  string j{PQgetvalue(res, 0, 0)};

  auto json = Json::parse(j);

  PQclear(res);
  return Blueprint::fromJson(json);
}

vector<Blueprint> DB::fetchBlueprints(string project)
{
  string q = fmt::format(
    "SELECT doc FROM blueprints where project = "
      "(SELECT id FROM projects WHERE name = '{pname}')",
    fmt::arg("pname", project)
  );

  PGresult *res = PQexec(conn_, q.c_str());
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "fetching project blueprints failed";
    LOG(INFO) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  vector<Blueprint> result;
  size_t n = PQntuples(res);
  for(size_t i=0; i<n; ++i)
  {
    string j{PQgetvalue(res, i, 0)};
    auto js = Json::parse(j);
    result.push_back(Blueprint::fromJson(js)); 
  }

  return result;
}


void DB::deleteBlueprint(string project, string bp_name)
{
 
  //if we can get a materialization without error
  try
  {
    fetchMaterialization(project, bp_name);
    throw DeleteActiveBlueprintError{};
  }
  catch(out_of_range&) { /*ok...*/ }

  stringstream ss;
  ss << "DELETE FROM blueprints where "
     << "project = "
        << "(SELECT id FROM projects WHERE name = " 
        << "'" << project << "') AND "
     << "(doc->>'name') = '" << bp_name << "'";

  string q = ss.str();

  PGresult *res = PQexec( conn_, q.c_str() );
  
  if(PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    LOG(ERROR) << "delete blueprint failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  PQclear(res);
}

// materialization +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

string DB::saveMaterialization(string project, string bpid, Json mzn)
{
  string s{"'"+mzn.dump()+"'"},
         p{"'"+project+"'"},
         b{"'"+bpid+"'"};

  stringstream ss;
  ss
    << "INSERT INTO materializations (blueprint, doc) values("
    <<    "( select id FROM blueprints WHERE "
    <<        "project = (SELECT id FROM projects WHERE name = "+p+") AND "
    <<        "(doc->>'name') = "+b
    <<    "), "+s+") "
    << "ON CONFLICT (blueprint) DO UPDATE SET doc = "+s+" "
    << "RETURNING id";

  string q = ss.str();

  PGresult *res = PQexec(conn_, q.c_str());

  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "save source failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  string id{PQgetvalue(res, 0, 0)};
  LOG(INFO) << "saved materialization " << id;

  PQclear(res);
  return id;
}

Blueprint DB::fetchMaterialization(string project, string bpid)
{
  string p{"'"+project+"'"},
         b{"'"+bpid+"'"};

  stringstream ss;
  ss
    << "SELECT doc FROM materializations "
    << "WHERE "
      << "blueprint = "
        << "(SELECT id FROM blueprints WHERE "
          << "(doc->>'name') = "+b
          << " AND "
          << "project = (SELECT id FROM projects WHERE name = "+p+")"
        << ")";

  string q = ss.str();

  PGresult *res = PQexec(conn_, q.c_str());
  
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "fetch materialization failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  if(PQntuples(res) == 0)
  {
    throw out_of_range{"materialization for ("+project+", "+bpid+") not found"};
  }

  Json j = Json::parse(string{PQgetvalue(res, 0, 0)});

  LOG(INFO) << "fetched materialization";

  PQclear(res);
  return Blueprint::fromJson(j);
}

vector<Blueprint> DB::fetchMaterializations(string project)
{
  string q = fmt::format(
    "SELECT doc FROM materializations where blueprint IN "
      "(SELECT id FROM blueprints WHERE project = "
        "(SELECT id FROM projects WHERE name = '{pname}')"
      ")",
    fmt::arg("pname", project)
  );

  PGresult *res = PQexec(conn_, q.c_str());
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "fetching project materializations failed";
    LOG(INFO) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  vector<Blueprint> result;
  size_t n = PQntuples(res);
  for(size_t i=0; i<n; ++i)
  {
    string j{PQgetvalue(res, i, 0)};
    auto js = Json::parse(j);
    result.push_back(Blueprint::fromJson(js)); 
  }

  return result;
}

void DB::deleteMaterialization(string project, string bpid)
{
  string p{"'"+project+"'"},
         b{"'"+bpid+"'"};

  stringstream ss;
  ss
    << "DELETE FROM materializations "
    << "WHERE "
      << "blueprint = "
        << "(SELECT id FROM blueprints WHERE "
          << "(doc->>'name') = "+b
          << " AND "
          << "project = (SELECT id FROM projects WHERE name = "+p+")"
        << ")";
  
  string q = ss.str();
  
  PGresult *res = PQexec( conn_, q.c_str() );
  
  if(PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    LOG(ERROR) << "delete materialization failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  PQclear(res);
}

// hardware topology +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      
void DB::setHwTopo(Json topo)
{
  string t{"'"+topo.dump()+"'"};

  stringstream ss;
  ss
    << "INSERT INTO hw_topology (doc) values("+t+") " 
    << "ON CONFLICT (id) DO UPDATE set doc = "+t+" "
    << "RETURNING id";

  string q = ss.str();

  PGresult *res = PQexec(conn_, q.c_str());
  
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "set hw topology failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  PQclear(res);
}

TestbedTopology DB::fetchHwTopo()
{
  string q{"SELECT doc FROM hw_topology"};
  
  PGresult *res = PQexec(conn_, q.c_str());
  
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "fetch hw topology failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }
  if(PQntuples(res) < 1)
  {
    string msg{"the testbed does not have a topology!?"};
    LOG(ERROR) << msg;
    PQclear(res);
    throw runtime_error{msg};
  }
  
  string j{PQgetvalue(res, 0, 0)};

  auto json = Json::parse(j);

  PQclear(res);
  return TestbedTopology::fromJson(json);
}

void DB::deleteHwTopo()
{
  throw runtime_error{"not implemented"};
}

size_t DB::newVxlanVni(string netid)
{
  string q = fmt::format(
    "INSERT INTO vxlan (netid) values('{}') RETURNING vni",
    netid
  );

  PGresult *res = PQexec(conn_, q.c_str());
  if(PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    LOG(ERROR) << "creating new vxlan vni failed";
    LOG(ERROR) << PQerrorMessage(conn_);
    PQclear(res);
    throw runtime_error{"pq query failure"};
  }

  string sv{PQgetvalue(res, 0, 0)};

  PQclear(res);
  return stoul(sv);
}

void freeVxlanVni(string /*netid*/)
{
  throw runtime_error{"not implemented"};
}
