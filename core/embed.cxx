#include "embed.hxx"
#include "topo.hxx"
#include "3p/pipes/pipes.hxx"
#include "common/net/glog.hxx"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fmt/format.h>

using std::vector;
using std::sort;
using std::make_pair;
using std::unordered_map;
using std::string;
using std::runtime_error;
using std::out_of_range;
using std::stringstream;
using std::endl;
using std::ostream;
using namespace marina;
using namespace pipes;

// Load ------------------------------------------------------------------------
bool Load::overloaded() const { return used > total; }

double Load::percentUsed() const
{
  if(total == 0) return 0;
  return static_cast<double>(used) / static_cast<double>(total);  
}

double Load::percentFree() const
{
  return 1.0 - percentUsed();
}
  
Load marina::operator + (Load x, Load y)
{
  Load z;
  z.used = x.used + y.used;
  z.total = x.total + y.total;
  return z;
}

// LoadVector ------------------------------------------------------------------
LoadVector LoadVector::operator+ (HwSpec x) const
{
  LoadVector v = *this;
  v.proc.used += x.proc;
  v.mem.used += x.mem;
  v.net.used += x.net;
  v.disk.used += x.disk;
  return v;
}

bool LoadVector::overloaded() const
{
  return 
    proc.overloaded() || 
    mem.overloaded() || 
    net.overloaded() || 
    disk.overloaded();
}

string Load::str()
{
  return fmt::format("[{}/{}] {}%", used, total, percentUsed()*100);
}

string LoadVector::str()
{
  return fmt::format(
    "proc: {}\n"
    "mem: {}\n"
    "net: {}\n"
    "disk: {}\n",
    proc.str(), mem.str(), net.str(), disk.str()
  );
}

HwSpec LoadVector::used()
{
  return HwSpec{proc.used, mem.used, net.used, disk.used};
}

HwSpec LoadVector::total()
{
  return HwSpec{proc.total, mem.total, net.total, disk.total};
}

double LoadVector::norm()
{
  return (
      proc.percentUsed() +
      mem.percentUsed() +
      net.percentUsed() +
      disk.percentUsed() ) / 4.0;

}

double LoadVector::inf_norm()
{
  auto x = used();
  return x.neg_inf_norm();
}

double LoadVector::free_inf_norm()
{
  auto x = total() - used();
  return x.inf_norm();
}

double LoadVector::free_norm()
{
  return (
      proc.percentFree() +
      mem.percentFree() +
      net.percentFree() +
      disk.percentFree() ) / 4.0;
}
  
LoadVector marina::operator + (LoadVector x, LoadVector y)
{
  LoadVector z;
  z.proc = x.proc + y.proc;
  z.mem = x.mem + y.mem;
  z.net = x.net + y.net;
  z.disk = x.disk + y.disk;
  return z;
}

LoadVector HostEmbedding::load() const
{
  LoadVector v;
  v.proc.total = host.cores();
  v.proc.used = machines
    | map<vector>([](auto x){ return x.cores(); }) 
    | reduce(plus);

  v.mem.total = host.memory().megabytes();
  v.mem.used = machines
    | map<vector>([](auto x){ return x.memory().megabytes(); })
    | reduce(plus);

  v.disk.total = host.disk().megabytes();
  v.disk.used = machines
    | map<vector>([](auto x){ return x.disk().megabytes(); })
    | reduce(plus);

  for(const auto & x : host.interfaces())
    v.net.total += x.second.capacity().megabits();

  v.net.used = machines
    | map<vector>([](auto x){ return x.hwspec().net; })
    | reduce(plus);

  return v;
}

  
size_t HE_Hash::operator() (const HostEmbedding &h)
{
  return HostSetHash{}(h.host);
}
  
bool HE_Cmp::operator() (const HostEmbedding &a, const HostEmbedding &b)
{
  return HostSetCMP{}(a.host, b.host);
}
  
size_t SE_Hash::operator() (const SwitchEmbedding &s)
{
  return SwitchSetHash{}(s.sw);
}
  
bool SE_Cmp::operator() (const SwitchEmbedding &a, const SwitchEmbedding &b)
{
  return SwitchSetCMP{}(a.sw, b.sw);
}

HostEmbedding HostEmbedding::operator+(Computer c)
{
  HostEmbedding x = *this;
  x.machines.insert(c);
  return x;
}

HostEmbedding::HostEmbedding(Host h)
  : host{h}
{}

SwitchEmbedding::SwitchEmbedding(Switch s)
  : sw{s}
{}

//EChart ----------------------------------------------------------------------
EChart::EChart(const TestbedTopology tt)
{
  for(auto h : tt.hosts()) hmap.emplace(h.second);
  for(auto s : tt.switches()) smap.emplace(s.second);
}

string EChart::overview()
{
  stringstream ss;
  for(auto h : hmap)
  {
    ss <<
      h.host.name() 
      << " " << h.machines.size() 
      << " " << h.load().norm() << endl;

    for(auto m : h.machines)
    {
      ss << "  " << m.name() << endl;
    }
  }

  for(auto s : smap)
  {
    ss <<
      s.sw.name() << " " << s.networks.size() << endl;
  }

  return ss.str();
}

HostEmbedding EChart::getEmbedding(Computer c)
{
  auto i = std::find_if(hmap.begin(), hmap.end(),
      [c](auto h)
      {
        return h.machines.find(c) != h.machines.end();
      });

  if(i != hmap.end()) return *i;
  else throw out_of_range{c.name()+"("+c.id().str()+") not found in echart"};
}

vector<SwitchEmbedding> EChart::getEmbedding(Network n)
{
  vector<SwitchEmbedding> ses;

  for(auto se : smap)
  {
    if(se.networks.find(n) != se.networks.end()) ses.push_back(se);
  }

  return ses;
}

EChart marina::embed(Blueprint b, EChart e, TestbedTopology tt)
{
  //sort the computers so we embed the 'lightest' first
  auto cs = b.computers() 
    | map<vector>([](auto x) { return x.second; })
    | sort([](auto x, auto y){ return x.hwspec().norm() < y.hwspec().norm(); });

  for(auto c : cs)
  {
    auto candidates = e.hmap 
      | map<vector>([c](auto x) { return x + c; })
      | sort([](auto x, auto y){ return x.load().norm() < y.load().norm(); });

    auto &chosen = candidates.front();

    if(chosen.load().overloaded())
    {
      string msg = fmt::format(
        "embedding failed \n"
        "most available host: {} \n"
        "{}",
        chosen.host.name(), chosen.load().str()
      );

      LOG(WARNING) << msg;

      throw runtime_error{msg};
    }

    e.hmap.erase(chosen);
    e.hmap.insert(chosen);
  }

  for(auto nw : b.networks())
  {
    unordered_map<Switch, size_t, SwitchSetHash, SwitchSetCMP> swc;
    for(auto nbr : nw.second.connections())
    {
      auto c = b.getComputerByMac(nbr.id);  
      auto he = e.getEmbedding(c);
      auto sws = tt.connectedSwitches(he.host);
      for(auto s : sws) swc[s]++;
    }

    for(auto p : swc)
    {
      if(p.second > 1)
      {
        auto i = e.smap.find(p.first);
        if(i == e.smap.end()) 
          throw runtime_error{"unknown switch: " + p.first.name()};

        SwitchEmbedding swe = *i;
        swe.networks.insert(nw.second);
        e.smap.erase(swe);
        e.smap.insert(swe);
      }
    }
  }

  return e;
}

EChart marina::unembed(Blueprint bp, EChart ec)
{
  for(const auto & c : bp.computers())
  {
    auto e = ec.getEmbedding(c.second);
    e.machines.erase(c.second);
    ec.hmap.erase(e);
    ec.hmap.insert(e);
  }
  
  for(const auto & n : bp.networks())
  {
    auto es = ec.getEmbedding(n.second);
    for(auto e : es) 
    {
      e.networks.erase(n.second);
      ec.smap.erase(e);
      ec.smap.insert(e);
    }
  }

  return ec;
}

