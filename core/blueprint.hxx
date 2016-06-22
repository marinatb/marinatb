#ifndef _LIBDNA_LIB_ENV_
#define _LIBDNA_LIB_ENV_

#include <memory>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <3p/json/src/json.hpp>
#include <core/util.hxx>

namespace marina
{
  //forward decl ...............................................................
  class Blueprint;
  class Network;
  class Computer;
  class Interface;
  using Json = nlohmann::json; 

  struct Computer_Guid_Hash
  {
    size_t operator() (const Computer &) const;
  };

  struct Computer_Guid_Cmp
  {
    bool operator() (const Computer &, const Computer &) const;
  };

  struct Network_Guid_Hash
  {
    size_t operator() (const Network &) const;
  };

  struct Network_Guid_Cmp
  {
    bool operator() (const Network &, const Network &) const;
  };
  
  struct Link
  {
    Link() = default;
    Link(std::pair<Computer,Interface>, Network);
    Link(Network, std::pair<Computer,Interface>);
    Link(Network, Network);

    //Link(Interface, Interface);
    //Link(std::string, std::string);
    static Link fromJson(Json);

    Json json() const;
    
    std::array<Endpoint, 2> endpoints;
  };


  // Blueprint -----------------------------------------------------------------
  class Blueprint
  {
    public:
      Blueprint(std::string);
      static Blueprint fromJson(Json);

      //name
      std::string name() const;
      Blueprint & name(std::string);

      //the project this blueprint is assigned to, may be empty initially
      std::string project() const;
      Blueprint & project(std::string);

      //id a UUID that uniquely identifies this blueprint in the universe of
      //all possible blueprints
      Uuid id() const;

      //component constructors
      Network network(std::string name);
      Computer computer(std::string name);

      using ComputerMap = std::unordered_map<Uuid, Computer, UuidHash, UuidCmp>;
      ComputerMap & computers() const;

      using NetworkMap = std::unordered_map<Uuid, Network, UuidHash, UuidCmp>;
      NetworkMap & networks() const;
      
      std::vector<Network> connectedNetworks(const Computer);
      //std::vector<Computer> connectedComputers(const Network);
      Computer & getComputer(std::string name) const;
      Network & getNetwork(std::string name) const;
      Network getNetworkById(const Uuid & id) const;
      Computer getComputerByMac(std::string mac) const;

      //component removal
      void removeComputer(std::string name);
      void removeNetwork(std::string name); 

      //component connection
      const std::vector<Link> & links() const;
      void connect(std::pair<Computer, Interface>, Network);
      void connect(Network, Network);

      //embedding
      Blueprint localEmbedding(std::string host_id);

      Json json() const;

      Blueprint clone() const;

    private:
      std::shared_ptr<struct Blueprint_> _;
  };
 
  // Neighbor ------------------------------------------------------------------
  struct Neighbor
  {
    enum class Kind { Network, Computer };

    Neighbor(Kind k, std::string id/*, Blueprint bp*/);
    static Neighbor fromJson(Json);

    Json json() const;
    
    Kind kind;
    std::string id;
    //Blueprint bp;
  };

  template <class T>
  T as(const Neighbor &);


  bool operator== (const Blueprint &, const Blueprint &);
  bool operator!= (const Blueprint &, const Blueprint &);
  
  // Bandwidth -----------------------------------------------------------------
  class Bandwidth
  {
    public:
      enum class Unit { BPS, KBPS, MBPS, GBPS, TBPS };
      Bandwidth() = default;
      Bandwidth(size_t, Unit);
      static Bandwidth fromJson(Json);
      static Unit parseUnit(std::string);

      size_t size{0};
      Unit unit{Unit::MBPS};

      size_t megabits() const ;
      Json json() const;
  };

  Bandwidth operator "" _bps(unsigned long long);
  Bandwidth operator "" _kbps(unsigned long long);
  Bandwidth operator "" _mbps(unsigned long long);
  Bandwidth operator "" _gbps(unsigned long long);
  Bandwidth operator "" _tbps(unsigned long long);

  bool operator == (const Bandwidth &, const Bandwidth &);
  bool operator != (const Bandwidth &, const Bandwidth &);

  // Latency -------------------------------------------------------------------
  class Latency
  {
    public:
      enum class Unit { NS, US, MS, S };
      Latency() = default;
      Latency(size_t, Unit);
      static Latency fromJson(Json);
      static Unit parseUnit(std::string);

      size_t size{0};
      Unit unit{Unit::MS};

      double toMS() const;

      Json json() const;
  };

  Latency operator "" _ns(unsigned long long);
  Latency operator "" _us(unsigned long long);
  Latency operator "" _ms(unsigned long long);
  Latency operator "" _s(unsigned long long);

  bool operator == (const Latency &, const Latency &);
  bool operator != (const Latency &, const Latency &);


  // IpV4Address ----------------------------------------------------------------

  class IpV4Address
  {
    public:
      IpV4Address() = default;
      IpV4Address(const std::string & addr, uint32_t mask);
      IpV4Address(const IpV4Address &) = default;
      IpV4Address(IpV4Address &&) = default;

      static IpV4Address fromJson(Json);
      Json json() const;

      IpV4Address & operator=(const IpV4Address &) = default;
      IpV4Address & operator=(IpV4Address &&) = default;

      std::string cidr() const,
                  addrStr() const;

      uint32_t addr() const;
      uint32_t mask() const;

      IpV4Address & operator++(int);
      IpV4Address & operator--(int);

      bool netZero() const;


    private:
      uint32_t addr_{0}, mask_{0};
      friend IpV4Address operator +(IpV4Address, uint32_t);
      friend IpV4Address operator -(IpV4Address, uint32_t);
  };
      
  IpV4Address operator +(IpV4Address, uint32_t);
  IpV4Address operator -(IpV4Address, uint32_t);

  std::ostream & operator<<(std::ostream &o, const IpV4Address &);

  // Network --------------------------------------------------------------------
  class Network
  {
    public:
      struct EmbeddingInfo
      {
        // vxlan network identifier
        size_t vni{0};
        std::unordered_set<std::string> switches;
      };

      Network(std::string);
      static Network fromJson(Json);

      //name
      Network & name(std::string);
      std::string name() const;
  
      //bandwidth
      const Bandwidth capacity() const;
      Network & capacity(Bandwidth);

      const IpV4Address & ipv4() const;
      Network & ipv4(std::string addr, uint32_t mask);

      //latency
      const Latency latency() const;
      Network & latency(Latency);

      //std::string guid() const;
      Uuid id() const;

      const std::vector<Neighbor> & connections() const;

      EmbeddingInfo & einfo() const;

      Json json() const;
      Network clone() const;

      friend Blueprint;

    private:
      std::shared_ptr<struct Network_> _;
  };

  bool operator == (const Network &, const Network &);
  bool operator != (const Network &, const Network &);
 
  // Memory --------------------------------------------------------------------
  class Memory
  {
    public:
      enum class Unit { B, KB, MB, GB, TB };
      Memory() = default;
      Memory(size_t, Unit);
      static Memory fromJson(Json j);
      static Unit parseUnit(std::string s);

      size_t size{0};
      Unit unit{Unit::B};

      Json json() const;
      size_t bytes() const;
      size_t megabytes() const;
  };

  Memory operator "" _b(unsigned long long);
  Memory operator "" _kb(unsigned long long);
  Memory operator "" _mb(unsigned long long);
  Memory operator "" _gb(unsigned long long);
  Memory operator "" _tb(unsigned long long);

  bool operator < (const Memory & a, const Memory & b);
  bool operator == (const Memory & a, const Memory & b);
  bool operator != (const Memory & a, const Memory & b);

  // Interface -----------------------------------------------------------------
  class Interface
  {
    public:
      struct EmbeddingInfo
      {
        //populated by the materializer if static networking is enabled
        IpV4Address ipaddr_v4;
      };

      Interface(std::string name);
      static Interface fromJson(Json);

      //name
      std::string name() const;
      Interface & name(std::string);

      //latency
      const Latency latency() const;
      Interface & latency(Latency);

      //capacity
      const Bandwidth capacity() const;
      Interface & capacity(Bandwidth);

      std::string mac() const;

      EmbeddingInfo & einfo() const;

      Json json() const;

      Interface clone() const;

    private:
      std::shared_ptr<struct Interface_> _;
  };

  bool operator == (const Interface &, const Interface &);
  bool operator != (const Interface &, const Interface &);

  // HwSpec -------------------------------------------------------------------
  struct HwSpec
  {
    HwSpec() = default;
    HwSpec(size_t proc, size_t mem, size_t net, size_t disk);

    size_t proc{0}, mem{0}, net{0}, disk{0};
    double norm(),
           inf_norm(),
           neg_inf_norm();
  };

  HwSpec operator+ (HwSpec, HwSpec);
  HwSpec operator- (HwSpec, HwSpec);

  // Computer ------------------------------------------------------------------
  class Computer
  {
    public:
      struct EmbeddingInfo
      {
        enum class LaunchState { None, Queued, Launching, Up };

        EmbeddingInfo() = default;
        EmbeddingInfo(std::string host, bool assigned);

        static EmbeddingInfo fromJson(Json);
        std::string host{"goblin"};
        bool assigned{false};
        LaunchState launch_state{LaunchState::None};

        Json json();
      };

      Computer(std::string name);
      static Computer fromJson(Json);
      
      //std::string guid() const;
      const Uuid & id() const;

      //name
      std::string name() const;
      Computer & name(std::string);

      //os
      std::string os() const;
      Computer & os(std::string); 

      //mem
      const Memory memory() const;
      Computer & memory(Memory);

      //disk
      const Memory disk() const;
      Computer & disk(Memory);

      //cores
      size_t cores() const;
      Computer & cores(size_t);

      //interfaces
      Interface ifx(std::string);
      Computer & add_ifx(std::string, Bandwidth, Latency = 0_ms);
      Computer & remove_ifx(std::string);
      std::unordered_map<std::string, Interface> & interfaces() const;
      Interface getInterfaceByMac(std::string) const;

      //embedding info
      EmbeddingInfo & embedding() const;
      Computer & embedding(EmbeddingInfo);

      HwSpec hwspec() const;

      Json json() const;

      Computer clone() const;
    private:
      std::shared_ptr<struct Computer_> _;
  };

  bool operator== (const Computer &, const Computer &);
  bool operator!= (const Computer &, const Computer &);
  bool isLinux(const Computer &);
 
}

namespace std
{
  string to_string(marina::Latency::Unit);
  string to_string(marina::Bandwidth::Unit);
  string to_string(marina::Memory);
  string to_string(marina::Memory::Unit);
}

#endif
