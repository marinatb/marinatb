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
    bool operator() (const Switch &, const Switch &) const;
  };

  struct SwitchSetHash
  {
    size_t operator() (const Switch &) const;
  };

  struct HostSetCMP
  {
    bool operator() (const Host &, const Host &) const;
  };

  struct HostSetHash
  {
    size_t operator() (const Host &) const;
  };

  /*
  struct Endpoint
  {
    enum class ComponentKind { Host, Switch };

    Endpoint(ComponentKind kind, std::string component_id, std::string mac);

    ComponentKind kind;
    std::string component_id, mac;
  };
  */

  class TestbedTopology
  {
    public:
      using SwitchSet = std::unordered_set<Switch, SwitchSetHash, SwitchSetCMP>;
      using HostSet = std::unordered_set<Host, HostSetHash, HostSetCMP>;

      TestbedTopology(std::string);

      std::string name() const;
      TestbedTopology & name(std::string);
      static TestbedTopology fromJson(Json);

      SwitchSet & switches() const;
      HostSet & hosts() const;

      Switch sw(std::string);
      Host host(std::string);

      HostSet connectedHosts(const Switch s);
      SwitchSet connectedSwitches(const Host h);

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

      std::vector<Network> & networks() const;
      void removeNetwork(std::string guid);

      //name
      std::string name() const;
      Switch & name(std::string);

      //std::vector<Host> & connectedHosts(TestbedTopology &) const;
      //std::vector<std::string> connectedHosts() const;


      //backplane
      Bandwidth backplane() const;
      Switch & backplane(Bandwidth);

      //allocated backplane
      Bandwidth allocatedBackplane() const;
      //Switch & allocatedBackplane(Bandwidth);

      Json json() const;
      Switch clone() const;

    private:
      friend TestbedTopology;
      void connect(Host, Switch, Bandwidth);
      std::shared_ptr<struct Switch_> _;
  };

  class Host
  {
    public:
      Host(std::string);
      static Host fromJson(Json);

      //TODO the const here is a bit disingenuous
      std::vector<Computer> & machines() const;
      void removeMachine(std::string cifx_mac);
      
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

      //network interfaces
      Interface ifx(std::string);
      Host & add_ifx(std::string, Bandwidth);
      Host & remove_ifx(std::string);

      //TODO the const here is a bit disingenuous
      std::unordered_map<std::string, Interface> & interfaces() const; 

      //resource management
      //LoadVector loadv() const;

      Json json() const;
      Host clone() const;

    private:
      std::shared_ptr<struct Host_> _;
  };

  bool operator== (const Switch & a, const Switch & b);
  bool operator!= (const Switch & a, const Switch & b);

  bool operator== (const Host & a, const Host & b);
  bool operator!= (const Host & a, const Host & b);
  
  /*
  struct MachineEmbedding
  {
    MachineEmbedding() = default;
    MachineEmbedding(std::string host, bool assigned);

    static MachineEmbedding fromJson(Json);
    std::string host{"goblin"};
    bool assigned{false};

    Json json();
  };

  struct NetEmbedding
  {
    NetEmbedding() = default;
    NetEmbedding(std::unordered_set<std::string> switches, bool assigned);

    std::unordered_set<std::string> switches;
    bool assigned{false};

    static NetEmbedding fromJson(Json);
    Json json();
  };
  */

}

#endif
