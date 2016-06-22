#include <sstream>
#include <random>
#include <uuid/uuid.h>
#include <stdexcept>
#include "util.hxx"
#include "glog/logging.h"

using std::string;
using std::stringstream;
using std::setfill;
using std::setw;
using std::hex;
using std::uniform_int_distribution;
using std::default_random_engine;
using std::random_device;
using std::exception;
using std::invalid_argument;
using std::runtime_error;
using std::out_of_range;
using std::function;
using std::experimental::optional;

using namespace marina;

string marina::generate_mac()
{
  stringstream ss;
  random_device r;
  default_random_engine gen(r());
  uniform_int_distribution<int> dist(0,255);
  ss << setfill('0') << setw(2) << hex << 2;

  for(size_t i=0; i<5; ++i)
    ss << ":" << setfill('0') << setw(2) << hex << dist(gen);

  return ss.str();
}

// Uuid -----------------------------------------------------------------------

Uuid::Uuid()
{
  uuid_generate(id);
}

string Uuid::str() const
{
  string s(36, '0');
  uuid_unparse(id, &s[0]);
  return s;
}

Json Uuid::json() const
{
  Json j;
  j["id"] = str();
  return j;
}

Uuid Uuid::fromJson(const Json &j)
{
  Uuid u;
  string s = extract(j, "id", "uuid");
  uuid_parse(s.c_str(), u.id);
  return u;
}

bool marina::operator==(const Uuid & a, const Uuid & b)
{
  return uuid_compare(a.id, b.id);
}

bool marina::operator!=(const Uuid & a, const Uuid & b)
{
  return !(a == b);
}

// Endpoint --------------------------------------------------------------------
  
Endpoint::Endpoint(Uuid id) : id{id} {}
Endpoint::Endpoint(Uuid id, optional<string> mac) : id{id}, mac{mac} {}

Json Endpoint::json() const
{
  Json j;
  j["id"] = id.json();

  if(mac)
  {
    j["mac"] = *mac;
  }
  return j;
}

Endpoint Endpoint::fromJson(const Json &j)
{
  Endpoint e;
  Json id_j = extract(j, "id", "endpoint");
  e.id = Uuid::fromJson(id_j);

  if(j.find("mac") != j.end())
  {
    string mac = extract(j, "mac", "endpoint");
    e.mac = mac;
  }
  return e;
}

bool marina::operator==(const Endpoint & a, const Endpoint & b)
{
  if(a.id != b.id) return false;
  if(a.mac)
  {
    if(!b.mac) return false;
    if(*a.mac != *b.mac) return false;
  }
  return true;
}

function<http::Response(http::Message)> 
marina::jsonIn(function<http::Response(Json)> f)
{
  //TODO 
  // not having this return type annotation combined with a semantic
  // error in the return statement (see (!!!) below) will cause 
  // clang 3.9 to go crashmuffin
  return [f](http::Message m) //-> http::Response
  {
    try 
    { 
      Json j = m.bodyAsJson(); 
      return f(j);
    }
    catch(invalid_argument & ex)
    { 
      LOG(ERROR) << "invalid json: " << ex.what();
      LOG(INFO) << "msg: " << m.bodyAsString();
      return http::Response{ http::Status::BadRequest(), "invalid json" }; 
      //(!!!) 
      //return http::XResponse{ http::Status::BadRequest(), "invalid json" }; 
    }
  };
}

http::Response marina::badRequest(string path, const Json & j, out_of_range &e)
{
  LOG(ERROR) << "[" << path << "] bad request";
  LOG(ERROR) << j.dump(2);
  LOG(ERROR) << e.what();
  return http::Response{ http::Status::BadRequest(), "" };
}

http::Response 
marina::unexpectedFailure(string path, const Json & j, exception &e)
{
  LOG(ERROR) << "[" << path << "] unexpected failure";
  LOG(ERROR) << j.dump(2);
  LOG(ERROR) << e.what();
  return http::Response{ http::Status::InternalServerError(), "" };
}

CmdResult marina::exec(string cmd)
{
  CmdResult result;
  char buffer[1024];
  //TODO: ghetto redirect, should do something better
  FILE *pipe = popen((cmd + " 2>&1").c_str(), "r");
  if(pipe == nullptr) 
  {
    LOG(ERROR) << "exec popen failed for `" << cmd << "`";
    throw runtime_error{"exec: popen failed"};
  }
  while(!feof(pipe))
  {
    if(fgets(buffer, 1024, pipe) != nullptr)
      result.output += buffer;
  }

  int pexit = pclose(pipe);
  result.code = WEXITSTATUS(pexit);
  return result;
}

