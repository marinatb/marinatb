#include <stdexcept>
#include <algorithm>
#include <unordered_set>
#include "topo.hxx"
#include "util.hxx"
#include "3p/pipes/pipes.hxx"

using std::runtime_error;
using std::out_of_range;
using std::string;
using std::unordered_set;
using std::hash;
using std::vector;
using std::array;
using std::set_difference;
using std::inserter;
using std::unordered_map;
using namespace marina;
using namespace pipes;

size_t SwitchSetHash::operator() (const Switch & s) 
{ 
  return hash<string>{}(s.name()); 
}

size_t HostSetHash::operator() (const Host & c) 
{ 
  return hash<string>{}(c.name()); 
}

bool SwitchSetCMP::operator() (const Switch & a, const Switch & b)
{
  SwitchSetHash h;
  return h(a) == h(b);
}

bool HostSetCMP::operator() (const Host & a, const Host & b)
{
  HostSetHash h;
  return h(a) == h(b);
}

bool marina::operator== (const Switch & a, const Switch & b)
{
  if(a.name() != b.name()) return false;
  if(a.backplane() != b.backplane()) return false;
  if(a.allocatedBackplane() != b.allocatedBackplane()) return false;

  if(a.connectedHosts().size() != b.connectedHosts().size()) return false;

  vector<Host> ahs = a.connectedHosts(),
               bhs = a.connectedHosts();

  auto cmp = [](const Host & x, const Host & y) { return x.name() < y.name(); };

  sort(ahs.begin(), ahs.end(), cmp);
  sort(bhs.begin(), bhs.end(), cmp);

  for(size_t i=0; i<ahs.size(); ++i)
  {
    if(ahs[i] != bhs[i]) return false;
  }

  return true;
}

bool marina::operator!= (const Switch & a, const Switch & b)
{
  return !(a == b);
}


bool marina::operator== (const Host & a, const Host & b)
{
  if(a.name() != b.name()) return false;
  if(a.cores() != b.cores()) return false;
  if(a.memory() != b.memory()) return false;
  if(a.disk() != b.disk()) return false;
  
  for(auto p : a.interfaces())
  {
    auto i = b.interfaces().find(p.first);
    if(i == b.interfaces().end()) return false;
    if(p.second != i->second) return false;
  }

  return true;
}

bool marina::operator!= (const Host & a, const Host & b)
{
  return !(a == b);
}

namespace marina {

  struct Endpoint
  {
    enum class Kind { Host, Switch };

    Endpoint(Kind kind, string name) : kind{kind}, name{name} {}
    Kind kind;
    string name;

    static Endpoint fromJson(Json j)
    {
      string ks = j.at("kind");
      Kind k;
      if(ks == "host") k = Kind::Host;
      else if(ks == "switch") k = Kind::Switch;
      else throw runtime_error{ks + " is not a valid endpoint kind"};

      return Endpoint{k, j.at("name")};
    }

    Json json() const
    {
      Json j;
      switch(kind)
      {
        case Kind::Host: j["kind"] = "host"; break;
        case Kind::Switch: j["kind"] = "switch"; break;
      }
      j["name"] = name;
      return j;
    }
  };

  struct TbLink
  {
    TbLink(Switch a, Switch b, Bandwidth capacity)
      : endpoints{{
          {Endpoint::Kind::Switch, a.name()},
          {Endpoint::Kind::Switch, b.name()}
        }},
        capacity{capacity}
    {}

    TbLink(Host a, Switch b, Bandwidth capacity)
      : endpoints{{
          Endpoint{Endpoint::Kind::Host, a.name()},
          Endpoint{Endpoint::Kind::Switch, b.name()}
        }},
        capacity{capacity}
    {}

    TbLink(Endpoint a, Endpoint b, Bandwidth c)
      : endpoints{{a, b}},
        capacity{c}
    {}

    static TbLink fromJson(Json j)
    {

      Json eps = j.at("endpoints");
      Endpoint a = Endpoint::fromJson(eps.at(0));
      Endpoint b = Endpoint::fromJson(eps.at(1));
      Bandwidth c = Bandwidth::fromJson(j.at("capacity"));

      return TbLink{a, b, c};
    }

    array<Endpoint,2> endpoints;
    Bandwidth capacity;

    Json json() const
    {
      Json j;
      j["endpoints"] = jtransform(endpoints);
      j["capacity"] = capacity.json();
      return j;
    }
  };

  struct TestbedTopology_
  {
    TestbedTopology_(string name) : name{name} {}

    string name;
    unordered_set<Switch, SwitchSetHash, SwitchSetCMP> switches;
    unordered_set<Host, HostSetHash, HostSetCMP> hosts;
    vector<TbLink> links;

    void removeEndpointLink(Endpoint::Kind, string name);
  };

  struct Switch_
  {
    Switch_(string name) : name{name} {} 

    string name;
    Bandwidth backplane, 
              allocated_backplane{0_gbps};

    vector<Host> connected_hosts;
  };

  struct Host_
  {
    Host_(string name) : host_comp{name} 
    {
      host_comp.add_ifx("ifx", 0_gbps);
    }

    Computer host_comp;
    vector<Computer> experiment_machines;
  };

}


// TestbedTopology -------------------------------------------------------------

TestbedTopology::TestbedTopology(string name) 
  : _{new TestbedTopology_{name}} 
{}

TestbedTopology TestbedTopology::fromJson(Json j)
{
  string name = j.at("name");
  TestbedTopology t{name};

  Json hosts = j.at("hosts");
  for(Json & hj : hosts)
  {
    Host h = Host::fromJson(hj);
    t._->hosts.insert(h);
  }

  Json switches = j.at("switches");
  for(Json & sj : switches)
  {
    Switch s = Switch::fromJson(sj);
    t._->switches.insert(s);
  }
  
  Json links = j.at("links");
  for(Json & lj : links)
  {
    TbLink l = TbLink::fromJson(lj);
    t._->links.push_back(l);
  }

  return t;
}

string TestbedTopology::name() const { return _->name; }
TestbedTopology & TestbedTopology::name(string name)
{
  _->name = name;
  return *this;
}

unordered_set<Switch, SwitchSetHash, SwitchSetCMP> & 
TestbedTopology::switches() const 
{ 
  return _->switches; 
}

unordered_set<Host, HostSetHash, HostSetCMP> & TestbedTopology::hosts() const 
{ 
  return _->hosts; 
}

Switch TestbedTopology::sw(string name)
{
  Switch s{name};
  _->switches.insert(s);
  return s;
}

Switch TestbedTopology::getSw(string name)
{
  Switch s{name};
  auto i = _->switches.find(s);
  if(i == _->switches.end())
    throw out_of_range{"switch "+name+" does not exist"};
  return *i;
}

Host TestbedTopology::getHost(string name)
{
  Host s{name};
  auto i = _->hosts.find(s);
  if(i == _->hosts.end())
    throw out_of_range{"host "+name+" does not exist"};
  return *i;
}

Host TestbedTopology::host(string name)
{
  Host c{name};
  _->hosts.insert(c);
  return c;
}

void TestbedTopology::connect(Switch a, Switch b, Bandwidth bw)
{
  _->links.push_back({a, b, bw});
}

void TestbedTopology::connect(Host a, Switch b, Bandwidth bw)
{
  _->links.push_back({a, b, bw});
  b._->connected_hosts.push_back(a);
}

Json TestbedTopology::json() const
{
  Json j;
  j["name"] = name();
  j["switches"] = jtransform(_->switches);
  j["hosts"] = jtransform(_->hosts);
  j["links"] = jtransform(_->links);
  return j;
}

void TestbedTopology_::removeEndpointLink(Endpoint::Kind kind, string name)
{
  links.erase(
    remove_if(links.begin(), links.end(),
      [kind, name](const TbLink & l){ 
        Endpoint a = l.endpoints[0],
                 b = l.endpoints[1];
        return
        (a.kind == kind && a.name == name) ||
        (b.kind == kind && b.name == name) ;
    }), links.end()
  );

}
      
void TestbedTopology::removeSw(string name)
{
  auto i = _->switches.erase(Switch{name});
  if(i == 0) return;

  _->removeEndpointLink(Endpoint::Kind::Switch, name);
}

void TestbedTopology::removeHost(string name)
{
  auto i = _->hosts.erase(Host{name});
  if(i == 0) return;

  _->removeEndpointLink(Endpoint::Kind::Host, name);
}

TestbedTopology TestbedTopology::clone() const
{
  TestbedTopology t{name()};
  for(auto & h : _->hosts) t._->hosts.insert(h.clone());
  for(auto & s : _->switches) t._->switches.insert(s.clone());
  return t;
}

bool marina::operator == (const TestbedTopology & a, const TestbedTopology & b)
{
  if(a.name() != b.name()) return false;

  if(a.switches().size() != b.switches().size()) return false;
  for(const auto & x : a.switches())
  {
    auto i = b.switches().find(x);
    if(i == b.switches().end()) return false;
    if(*i != x) return false;
  }
  
  if(a.hosts().size() != b.hosts().size()) return false;
  for(const auto & x : a.hosts())
  {
    auto i = b.hosts().find(x);
    if(i == b.hosts().end()) return false;
    if(*i != x) return false;
  }

  return true;
}

bool marina::operator != (const TestbedTopology & a, const TestbedTopology & b)
{
  return !(a == b);
}

// Switch ----------------------------------------------------------------------

Switch::Switch(string name)
  : _{new Switch_{name}}
{}

string Switch::name() const { return _->name; }
Switch & Switch::name(string name)
{
  _->name = name;
  return *this;
}

std::vector<Host> & Switch::connectedHosts() const
{
  return _->connected_hosts;
}

Bandwidth Switch::backplane() const { return _->backplane; }
Switch & Switch::backplane(Bandwidth b)
{
  _->backplane = b;
  return *this;
}

Bandwidth Switch::allocatedBackplane() const { return _->allocated_backplane; }
Switch & Switch::allocatedBackplane(Bandwidth b)
{
  _->allocated_backplane = b;
  return *this;
}

Switch Switch::fromJson(Json j)
{
  string name = j.at("name");
  Switch s{name};
  s.backplane(Bandwidth::fromJson(j.at("backplane")));
  s.allocatedBackplane(Bandwidth::fromJson(j.at("allocated-backplane")));
  
  Json chs = j.at("connected-hosts");
  for(const Json & hj : chs)
  {
    s.connectedHosts().push_back(Host::fromJson(hj));
  }

  return s;
}

Json Switch::json() const
{
  Json j;
  j["name"] = name();
  j["backplane"] = backplane().json();
  j["allocated-backplane"] = allocatedBackplane().json();
  j["connected-hosts"] = jtransform(connectedHosts());

  return j;
}

Switch Switch::clone() const
{
  Switch s{_->name};
  s._->backplane = _->backplane;
  s._->allocated_backplane = _->allocated_backplane;
  s._->connected_hosts = _->connected_hosts 
    | map([](auto x){ return x.clone(); });

  return s;
}

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

// Host ------------------------------------------------------------------------

Host::Host(string name)
  : _{new Host_{name}}
{}



string Host::name() const { return _->host_comp.name(); }
Host & Host::name(string name)
{
  _->host_comp.name(name);
  return *this;
}

const Memory Host::memory() const { return _->host_comp.memory(); }
Host & Host::memory(Memory m)
{
  _->host_comp.memory(m);
  return *this;
}

const Memory Host::disk() const { return _->host_comp.disk(); }
Host & Host::disk(Memory m)
{
  _->host_comp.disk(m);
  return *this;
}

size_t Host::cores() const { return _->host_comp.cores(); }
Host & Host::cores(size_t n)
{
  _->host_comp.cores(n);
  return *this;
}

/*
const Interface Host::ifx() const
{
  return _->host_comp.ifx("ifx");
}

unordered_map<string, Interface> & Host::interfaces() const
{
  return _->host_comp.interfaces();
}

Host & Host::ifx(Bandwidth b)
{
  _->host_comp.ifx("ifx").capacity(b);
  return *this;
}
*/

Interface Host::ifx(string name)
{
  return _->host_comp.ifx(name);
}

Host & Host::add_ifx(string name, Bandwidth bw)
{
  _->host_comp.add_ifx(name, bw, 0_ms);
  return *this;
}

Host & Host::remove_ifx(string name)
{
  _->host_comp.remove_ifx(name);
  return *this;
}

unordered_map<string, Interface> & Host::interfaces() const
{
  return _->host_comp.interfaces();
}


LoadVector Host::loadv() const
{
  LoadVector v;
  v.proc.total = cores();
  v.proc.used = experimentMachines()
    | map([](auto x){ return x.cores(); }) 
    | reduce(plus);

  v.mem.total = memory().bytes();
  v.mem.used = experimentMachines()
    | map([](auto x){ return x.memory().bytes(); })
    | reduce(plus);

  v.disk.total = disk().megabytes();
  v.mem.used = experimentMachines()
    | map([](auto x){ return x.disk().bytes(); })
    | reduce(plus);

  for(const auto & x : interfaces())
    v.net.total += x.second.capacity().megabits();

  //v.net.total = ifx().capacity().megabits();
  v.net.used = experimentMachines()
    | map([](auto x){ return x.hwspec().net; })
    | reduce(plus);

  return v;
}

vector<Computer> & Host::experimentMachines() const 
{ 
  return _->experiment_machines; 
}

Host Host::fromJson(Json j)
{
  string name = j.at("name");
  Host h{name};
  
  h.cores(j.at("cores"))
   .memory(Memory::fromJson(j.at("memory")))
   .disk(Memory::fromJson(j.at("disk")));

  Json ifxs = j.at("interfaces");
  for(const Json & ij : ifxs)
  {
    Interface ifx = Interface::fromJson(ij);
    h.interfaces().insert_or_assign(ifx.name(), ifx);
  }

  Json xpns = j.at("experiment-nodes");
  for(const Json & xj : xpns)
  {
    h.experimentMachines().push_back(Computer::fromJson(xj));
  }

  return h;
}

Json Host::json() const
{
  Json j;
  j["name"] = name();
  j["cores"] = cores();
  j["memory"] = memory().json();
  j["disk"] = disk().json();
  j["experiment-nodes"] = jtransform(_->experiment_machines);
  j["interfaces"] = jtransform(interfaces());
  return j;
}

Host Host::clone() const
{
  Host h{name()};
  h._->host_comp = _->host_comp.clone();
  for(auto & c : _->experiment_machines)
  { 
    h._->experiment_machines.push_back(c.clone()); 
  }
  return h;
}

// Embedding -------------------------------------------------------------------
Embedding::Embedding(string h, bool a)
  : host{h},
    assigned{a}
{}

Embedding Embedding::fromJson(Json j)
{
  string host = j.at("host");
  bool assigned = j.at("assigned");
  return Embedding{host, assigned};
}

Json Embedding::json()
{
  Json j;
  j["host"] = host;
  j["assigned"] = assigned;

  return j;
}
