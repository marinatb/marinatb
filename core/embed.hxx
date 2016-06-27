#ifndef MARINATB_EMBED_HXX
#define MARINATB_EMBED_HXX

#include <vector>
#include "topo.hxx"

namespace marina {

struct HostEmbedding;
struct SwitchEmbedding;
struct Load;
struct LoadVector;

struct EChart
{
  EChart(const TestbedTopology);

  std::unordered_map<Uuid, HostEmbedding, UuidHash, UuidCmp> hmap;
  std::unordered_map<Uuid, SwitchEmbedding, UuidHash, UuidCmp> smap;

  HostEmbedding getEmbedding(Computer);
  std::vector<SwitchEmbedding> getEmbedding(Network);

  std::string overview();
};

EChart embed(Blueprint, EChart, TestbedTopology);
EChart unembed(Blueprint, EChart);

struct HostEmbedding
{
  HostEmbedding(Host);

  Host host;
  std::unordered_map<Uuid, Computer, UuidHash, UuidCmp> machines;

  LoadVector load() const;
  HostEmbedding operator+(Computer);
  HostEmbedding operator-(Computer);
};

struct SwitchEmbedding
{
  SwitchEmbedding(Switch s);

  Switch sw;
  std::unordered_map<Uuid, Network, UuidHash, UuidCmp> networks;
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


}

#endif
