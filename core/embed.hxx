#ifndef DNA_ENV_EMBED
#define DNA_ENV_EMBED

#include <vector>
#include "topo.hxx"

namespace marina {

/*
 * Embedding Chart
 * ---------------
 *
 * 
 *
 */

struct HostEmbedding;
struct SwitchEmbedding;

struct HE_Hash
{
  size_t operator() (const HostEmbedding &);
};

struct HE_Cmp
{
  bool operator() (const HostEmbedding &, const HostEmbedding &);
};

struct SE_Hash
{
  size_t operator() (const SwitchEmbedding &);
};

struct SE_Cmp
{
  bool operator() (const SwitchEmbedding &, const SwitchEmbedding &);
};
  
struct Load
{
  Load() = default;
  Load(size_t used, size_t total) : used{used}, total{total} {}
  size_t used{0}, total{0};

  double percentFree() const, 
          percentUsed() const;

  bool overloaded() const;
  std::string str();
};

Load operator + (Load, Load);

struct LoadVector
{
  Load proc, mem, net, disk;    

  LoadVector operator+ (HwSpec) const;

  bool overloaded() const;
  HwSpec used(),
          total();

  double norm(),
          inf_norm(),
          free_norm(),
          free_inf_norm();

  std::string str();
};

LoadVector operator + (LoadVector, LoadVector);

struct HostEmbedding
{
  HostEmbedding(Host);

  Host host;
  std::unordered_map<Uuid, Computer, UuidHash, UuidCmp> machines;

  LoadVector load() const;
  HostEmbedding operator+(Computer);
};

struct SwitchEmbedding
{
  SwitchEmbedding(Switch s);

  Switch sw;
  std::unordered_map<Uuid, Network, UuidHash, UuidCmp> networks;
};

struct EChart
{
  EChart(const TestbedTopology);
  std::unordered_set<HostEmbedding, HE_Hash, HE_Cmp> hmap;
  std::unordered_set<SwitchEmbedding, SE_Hash, SE_Cmp> smap;

  HostEmbedding getEmbedding(Computer);
  std::vector<SwitchEmbedding> getEmbedding(Network);

  std::string overview();
};

EChart embed(Blueprint, EChart, TestbedTopology);
EChart unembed(Blueprint, EChart);

}



#endif
