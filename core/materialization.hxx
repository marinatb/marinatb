#ifndef MARINATB_MZN_HXX
#define MARINATB_MZN_HXX

#include <unordered_map>
#include <mutex>
#include "core/util.hxx"
#include "core/blueprint.hxx"

namespace marina
{
  // forward decl
  struct Materialization;
  struct MzMap;
  struct InterfaceMzInfo;
  struct ComputerMzInfo;
  struct NetworkMzInfo;


  struct Materialization
  {
    //std::unordered_map<Uuid, ComputerMzInfo, UuidHash, UuidCmp> machines;
    UuidMap<ComputerMzInfo> machines;
    std::unordered_map<Uuid, NetworkMzInfo, UuidHash, UuidCmp> networks;
    std::mutex mtx;
  };

  struct MzMap
  {
    public:
      Materialization & get(Uuid id);

    private:
      std::unordered_map<Uuid, Materialization, UuidHash, UuidCmp> data_;
      std::mutex mtx_;
  };
  
  struct InterfaceMzInfo
  {
    IpV4Address ipaddr_v4;

    Json json() const;
    static InterfaceMzInfo fromJson(Json);
  };

  struct ComputerMzInfo
  {
    enum class LaunchState { None, Queued, Launching, Up };
    LaunchState launchState;
    std::unordered_map<std::string, InterfaceMzInfo> interfaces;

    Json json() const;
    static ComputerMzInfo fromJson(Json);
  };

  struct NetworkMzInfo
  {
    size_t vni;
  };

}

#endif
