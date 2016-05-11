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
using std::function;

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

string marina::generate_guid()
{
  uuid_t x;
  uuid_generate(x);
  string s(36, '0');
  uuid_unparse(x, &s[0]);
  return s;
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

http::Response marina::badRequest(string path, Json & j)
{
  LOG(ERROR) << "[" << path << "] bad request";
  LOG(ERROR) << j.dump(2);
  return http::Response{ http::Status::BadRequest(), "" };
}

http::Response marina::unexpectedFailure(string path, Json & j, exception &e)
{
  LOG(ERROR) << "[" << path << "] unexpected failure";
  LOG(ERROR) << j.dump(2);
  LOG(ERROR) << e.what();
  return http::Response{ http::Status::InternalServerError(), "" };
}
