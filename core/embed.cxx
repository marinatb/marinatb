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
using std::reverse;
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
    | map<vector>([](const auto & x){ return x.second.cores(); }) 
    | reduce(plus);

  v.mem.total = host.memory().megabytes();
  v.mem.used = machines
    | map<vector>([](auto x){ return x.second.memory().megabytes(); })
    | reduce(plus);

  v.disk.total = host.disk().megabytes();
  v.disk.used = machines
    | map<vector>([](auto x){ return x.second.disk().megabytes(); })
    | reduce(plus);

  for(const auto & x : host.interfaces())
    v.net.total += x.second.capacity().megabits();

  v.net.used = machines
    | map<vector>([](auto x){ return x.second.hwspec().net; })
    | reduce(plus);

  return v;
}

HostEmbedding HostEmbedding::operator+(Computer c)
{
  HostEmbedding x = *this;
  x.machines.insert_or_assign(c.id(), c);
  return x;
}

HostEmbedding HostEmbedding::operator-(Computer c)
{
  HostEmbedding x = *this;
  x.machines.erase(c.id());
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
  for(auto h : tt.hosts()) hmap.insert_or_assign(h.second.id(), h.second);
  for(auto s : tt.switches()) smap.insert_or_assign(s.second.id(), s.second);
}

string EChart::overview()
{
  stringstream ss;
  for(const auto & p : hmap)
  {
    const auto &h  = p.second;

    ss <<
      h.host.name() 
      << " " << h.machines.size() 
      << " " << h.load().norm() << endl;

    for(auto m : h.machines)
    {
      ss << "  " << m.second.name() << endl;
    }
  }

  for(const auto & p : smap)
  {
    const auto & s = p.second;
    ss <<
      s.sw.name() << " " << s.networks.size() << endl;
  }

  return ss.str();
}

HostEmbedding EChart::getEmbedding(Computer c)
{
  auto i = std::find_if(hmap.begin(), hmap.end(),
      [c](const auto & p)
      {
        const auto &h = p.second;
        return h.machines.find(c.id()) != h.machines.end();
      });

  if(i != hmap.end()) return i->second;
  else throw out_of_range{c.name()+"("+c.id().str()+") not found in echart"};
}

vector<SwitchEmbedding> EChart::getEmbedding(Network n)
{
  vector<SwitchEmbedding> ses;

  for(const auto & p : smap)
  {
    const auto & se = p.second;
    if(se.networks.find(n.id()) != se.networks.end()) ses.push_back(se);
  }

  return ses;
}

void pack(HostEmbedding & he, vector<Computer> & cs)
{
  while(!cs.empty())
  {
    he = he + cs.back();
    if(he.load().overloaded()) 
    {
      he = he - cs.back();
      break;
    }
    cs.pop_back();
  }
}

EChart marina::embed(Blueprint b, EChart e, TestbedTopology tt)
{

  //sort the networks from largest to smallest
  auto nets = b.networks()
    | map<vector>([b](const auto &x)
      { 
        return make_pair(x.second, b.connectedComputers(x.second)); 
      })
    | sort([b](const auto & x, const auto & y)
      { 
        return x.second.size() > y.second.size();
      });

  //create a vector of computers in the above network sorted order
  vector<Computer> cs;
  for(const auto & n : nets)
  {
    for(const auto & c : n.second)
    {
      auto i = find_if(cs.begin(), cs.end(),
          [c](const auto & ix)
          {
            return ix.id() == c.first.id();
          });

      if(i == cs.end()) cs.push_back(c.first);
    }
  }

  //reverse the order for end popping
  reverse(cs.begin(), cs.end());

  auto aggLoad = [](const auto & hosts)
  {
    double agg = 
    hosts
    | map<vector>([](const auto & x)
      {
        return x.load().free_norm();
      })
    | reduce(plus);

    return agg / hosts.size();
  };

  //sort the switches based on aggregate load
  auto sws = e.smap
    | map<vector>([tt,e](const auto & p)
      {
        const auto & x = p.second;
        return make_pair(
            x, 
            tt.connectedHosts(x.sw) 
            | map<vector>([e](const auto &y) 
              { 
                return e.hmap.find(y.id())->second; 
              })
          );
      })
    | sort([e, aggLoad](const auto & x, const auto & y)
      { 
        return aggLoad(x.second) < aggLoad(y.second); 
      });

  //proceed with the embedding in the above switch sorted order
  for(auto & p : sws)
  {
    while(!cs.empty())
    {
      sort(p.second.begin(), p.second.end(),
        [](const auto & x, const auto & y) 
        { 
          return x.load().free_norm() > y.load().free_norm();
        });

      auto & chosen = p.second.front();
      size_t before = cs.size();
      pack(chosen, cs);
      e.hmap.insert_or_assign(chosen.host.id(), chosen);
      size_t after = cs.size();
      if(before == after) break;
    }
  }

  if(!cs.empty()) throw runtime_error{"embedding failed"};

  for(auto nw : b.networks())
  {
    unordered_map<Uuid, size_t, UuidHash, UuidCmp> swc;
    auto cxs = b.connectedComputers(nw.second);
    for(const auto & p : cxs)
    {
      const Computer & c = p.first;
      auto he = e.getEmbedding(c);
      auto sws = tt.connectedSwitches(he.host);
      for(auto s : sws) swc[s.id()]++;
    }

    for(auto p : swc)
    {
      if(p.second > 1)
      {
        auto i = e.smap.find(p.first);
        if(i == e.smap.end()) 
          throw runtime_error{"unknown switch id: " + p.first.str()};

        SwitchEmbedding swe = i->second;
        swe.networks.insert_or_assign(nw.second.id(), nw.second);
        e.smap.insert_or_assign(swe.sw.id(), swe);
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
    e.machines.erase(c.second.id());
    ec.hmap.insert_or_assign(e.host.id(), e);
  }
  
  for(const auto & n : bp.networks())
  {
    auto es = ec.getEmbedding(n.second);
    for(auto e : es) 
    {
      e.networks.erase(n.second.id());
      ec.smap.insert_or_assign(e.sw.id(), e);
    }
  }

  return ec;
}

