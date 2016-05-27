#ifndef LIBDNA_ENV_UTIL
#define LIBDNA_ENV_UTIL

#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "3p/json/src/json.hpp"
#include "common/net/proto.hxx"
#include "common/net/http_server.hxx"

namespace marina {

using Json = nlohmann::json; 

template <class T, class F>
inline
std::vector<Json> jtransform(T && xs, F && f)
{
  std::vector<Json> js;
  js.reserve(xs.size());
  transform( xs.begin(), xs.end(), back_inserter(js), f);
  return js;
}

template <class T>
inline
std::vector<Json> jtransform(const std::unordered_map<std::string, T> & xs)
{
  return jtransform(xs,
    [](const std::pair<std::string,T> & p){ return p.second.json(); } 
  );
}

template <class C>
inline
std::vector<Json> jtransform(const C & xs)
{
  return jtransform(xs,
    [](const typename C::value_type & x){ return x.json(); } 
  );
}

std::string generate_mac();
std::string generate_guid();

std::function<http::Response(http::Message)> 
jsonIn(std::function<http::Response(Json)>);

http::Response badRequest(std::string path, Json & j, std::out_of_range &e);
http::Response unexpectedFailure(std::string path, Json & j, std::exception &e);

struct CmdResult
{
  std::string output;
  int code{0};
};

CmdResult exec(std::string cmd);

template <class Key, class Value>
class LinearIdCacheMap
{
  public:
    Value create(Key key)
    {
      if(m_.find(key) != m_.end()) return m_.at(key);

      return m_[key] = v_++;
    }

    Value get(Key key) { return m_.at(key); }

  private:
    Value v_{};
    std::unordered_map<Key, Value> m_;
};

struct RtReq
{
  nlmsghdr header;
  ifinfomsg msg;
};

std::string mac_2_ifname(std::string mac);


}

#endif
