#ifndef LIBDNA_ENV_UTIL
#define LIBDNA_ENV_UTIL

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <experimental/optional>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <uuid/uuid.h>
#include "3p/json/src/json.hpp"
#include "common/net/proto.hxx"
#include "common/net/http_server.hxx"

namespace marina {

using Json = nlohmann::json; 

struct Uuid
{
  Uuid();

  std::string str() const;
  Json json() const;
  static Uuid fromJson(const Json &j);
  
  uuid_t id;
  
};

bool operator==(const Uuid &, const Uuid &);
bool operator!=(const Uuid &, const Uuid &);

struct UuidHash
{
  size_t operator()(const Uuid &u) const
  {
    return std::hash<std::string>{}(u.str());
  }
};

struct UuidCmp
{
  bool operator()(const Uuid &a, const Uuid &b) const
  {
    return uuid_compare(a.id, b.id) == 0;
  }
};

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

template <class T>
inline
std::vector<Json> jtransform(const std::unordered_map<Uuid, T, UuidHash, UuidCmp> & xs)
{
  return jtransform(xs,
    [](const std::pair<Uuid,T> & p){ return p.second.json(); } 
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


struct Endpoint
{
  Endpoint() = default;
  Endpoint(Uuid);
  Endpoint(Uuid, std::experimental::optional<std::string>);

  Uuid id;
  std::experimental::optional<std::string> mac;

  Json json() const;
  static Endpoint fromJson(const Json &);
};

bool operator==(const Endpoint &, const Endpoint &);

std::string generate_mac();

std::function<http::Response(http::Message)> 
jsonIn(std::function<http::Response(Json)>);

http::Response 
badRequest(std::string path, const Json & j, std::out_of_range &e);

http::Response 
unexpectedFailure(std::string path, const Json & j, std::exception &e);

struct CmdResult
{
  std::string output;
  int code{0};
};

CmdResult exec(std::string cmd);

template <class Key, class Value, class ...TT>
class LinearIdCacheMap
{
  public:
    Value create(Key key)
    {
      lk_.lock();
      if(m_.find(key) != m_.end()) 
      {
        auto v = m_.at(key);
        lk_.unlock();
        return v;
      }
      lk_.unlock();

      return m_[key] = v_++;
    }

    Value get(Key key) 
    { 
      lk_.lock();
      Value v = m_.at(key);
      lk_.unlock();
      return v;
    }

    void erase(Key key)
    {
      lk_.lock();
      m_.erase(key);
      lk_.unlock();
    }

  private:
    std::mutex mtx_;
    std::unique_lock<std::mutex> lk_{mtx_, std::defer_lock_t{}};
    Value v_{};
    std::unordered_map<Key, Value, TT...> m_;
};

struct RtReq
{
  nlmsghdr header;
  ifinfomsg msg;
};

std::string mac_2_ifname(std::string mac);

inline auto extract(Json j, std::string tag, std::string context)
{
  try { return j.at(tag); }
  catch(...) 
  { 
    LOG(ERROR) << j;
    throw std::out_of_range{"error extracting " + context+":"+tag};
  }
}

}

#endif
