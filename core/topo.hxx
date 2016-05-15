#ifndef _LIBDNA_LIB_TOPO_
#define _LIBDNA_LIB_TOPO_

#include <string>
#include <memory>
#include <unordered_set>
#include <3p/json/src/json.hpp>
#include "blueprint.hxx"

namespace marina {
  class Switch;
  class Host;
}

namespace std
{
  // Switch set machinery
  template<>
  struct hash<marina::Switch>
  {
    size_t operator() (const marina::Switch & s);
  };

  // Host set machinery
  template<>
  struct hash<marina::Host>
  {
    size_t operator() (const marina::Host & c);
  };
}

namespace marina { 

  class Switch;
  class Host;
  using Json = nlohmann::json; 

  struct SwitchSetCMP
  {
    bool operator() (const Switch &, const Switch &);
  };

  struct SwitchSetHash
  {
    size_t operator() (const Switch &);
  };

  struct HostSetCMP
  {
    bool operator() (const Host &, const Host &);
  };

  struct HostSetHash
  {
    size_t operator() (const Host &);
  };

  class TestbedTopology
  {
    public:
      TestbedTopology(std::string);

      std::string name() const;
      TestbedTopology & name(std::string);
      static TestbedTopology fromJson(Json);

      std::unordered_set<Switch, SwitchSetHash, SwitchSetCMP> & 
      switches() const;

      std::unordered_set<Host, HostSetHash, HostSetCMP> & 
      hosts() const;

      Switch sw(std::string);
      Host host(std::string);

      void removeSw(std::string);
      void removeHost(std::string);

      Switch getSw(std::string);
      Host getHost(std::string);

      void connect(Switch, Switch, Bandwidth);
      void connect(Host, Switch, Bandwidth);

      Json json() const;

      TestbedTopology clone() const;
    private:
      std::shared_ptr<struct TestbedTopology_> _;
  };

  bool operator == (const TestbedTopology &, const TestbedTopology &);
  bool operator != (const TestbedTopology &, const TestbedTopology &);
 
  class Switch
  {
    public:
      Switch(std::string);
      static Switch fromJson(Json);

      //name
      std::string name() const;
      Switch & name(std::string);

      std::vector<Host> & connectedHosts() const;

      //backplane
      Bandwidth backplane() const;
      Switch & backplane(Bandwidth);

      //allocated backplane
      Bandwidth allocatedBackplane() const;
      Switch & allocatedBackplane(Bandwidth);

      Json json() const;
      Switch clone() const;
    private:
      friend TestbedTopology;
      void connect(Host, Switch, Bandwidth);
      std::shared_ptr<struct Switch_> _;
  };

  struct Load
  {
    Load() = default;
    Load(size_t used, size_t total) : used{used}, total{total} {}
    size_t used{0}, total{0};

    double percentFree() const, 
           percentUsed() const;

    bool overloaded() const;
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
  };

  LoadVector operator + (LoadVector, LoadVector);

  class Host
  {
    public:
      Host(std::string);
      static Host fromJson(Json);

      //TODO the const here is a bit disingenuous
      std::vector<Computer> & experimentMachines() const;
      
      //name
      std::string name() const;
      Host & name(std::string);

      //cores
      size_t cores() const;
      Host & cores(size_t);
      
      //mem
      const Memory memory() const;
      Host & memory(Memory);

      //disk
      const Memory disk() const;
      Host & disk(Memory);

      //network interface
      const Interface ifx() const;
      Host & ifx(Bandwidth);

      //TODO the const here is a bit disingenuous
      std::unordered_map<std::string, Interface> & interfaces() const; 

      //resource management
      LoadVector loadv() const;

      Json json() const;
      Host clone() const;
    private:
      std::shared_ptr<struct Host_> _;
  };

  bool operator== (const Switch & a, const Switch & b);
  bool operator!= (const Switch & a, const Switch & b);

  bool operator== (const Host & a, const Host & b);
  bool operator!= (const Host & a, const Host & b);
  
  struct Embedding
  {
    Embedding() = default;
    Embedding(std::string host, bool assigned);

    static Embedding fromJson(Json);
    std::string host{"goblin"};
    bool assigned{false};

    Json json();
  };

}

#endif
