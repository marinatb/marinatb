#include "topologies.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using std::to_string;
using namespace marina;
using namespace pipes;
using namespace std::literals::string_literals;

//string marina::operator+ (string s, int i) { return s + to_string(i); }

TestbedTopology marina::deter2015()
{
  TestbedTopology t{"isi"};

  //root switch
  auto motherswitch = t.sw("motherswitch").backplane(5_tbps);

  //generation 1 cluster
  auto g1trunk = t.sw("g1trunk").backplane(550_gbps);
  t.connect(g1trunk, motherswitch, 10_gbps);

  //microcloud ****************
  //switches
  auto mc_switches = 
  range(0,4)
    | map([](int x){ return to_string(x); })
    | map([&t](string x){ return t.sw("mcs"+x).backplane(550_gbps); })
    | for_each([&t,&g1trunk](Switch x){ t.connect(x, g1trunk, 20_gbps); });

  //hosts
  range(0,128)
    | for_each([&t,&mc_switches](int x){ 
         auto c = t.host("mc"+to_string(x))
           .cores(4)
           .memory(16_gb)
           .disk(200_gb) 
           .add_ifx("ethX", 6_gbps); //TODO XXX

         t.connect(c, mc_switches[x/32], 6_gbps); });

  //hp ************************
  //switches
  auto hp_switches = 
  range(0,5)
    | map([](int x){ return to_string(x); })
    | map([&t](string x){ return t.sw("g2s"+x).backplane(1280_gbps); })
    | for_each([&t,&motherswitch](Switch x) 
         { t.connect(x, motherswitch, 160_gbps); });

  //hosts
  range(0,110)
    | for_each([&t,&hp_switches](int x){
        auto c = t.host("hp"+to_string(x))
          .cores(12)
          .memory(24_gb)
          .disk(1_tb)
          .add_ifx("ethX", 20_gbps); //TODO XXX

        t.connect(c, hp_switches[x/22], 20_gbps); });

  //pc2133 ********************
  //switches
  auto pc2133_switches = 
  range(0,3)
    | map([&t,&g1trunk](int x) 
      { 
        auto s = t.sw("pc2133s"+to_string(x)).backplane(550_gbps);
        t.connect(s, g1trunk, 20_gbps);
        return s;
      });

  //hosts
  range(0,64)
    | for_each([&t,&pc2133_switches](int x){
        auto c = t.host("pc2133_"+to_string(x))
          .cores(4)
          .memory(4_gb)
          .disk(250_gb)
          .add_ifx("ethX", 4_gbps); //TODO XXX

        t.connect(c, pc2133_switches[x/27], 4_gbps); });

  //pc 3060 *******************
  //switch
  auto pc3060_switch = t.sw("pc3060s0").backplane(550_gbps);
  t.connect(pc3060_switch, g1trunk, 20_gbps);
  
  //hosts
  range(0,18)
    | for_each([&t,&pc3060_switch](int x){
        auto c = t.host("pc3060_"+to_string(x))
          .cores(4)
          .memory(4_gb)
            .disk(36_gb)
          .add_ifx("ethX", 4_gbps); //TODO XXX

        t.connect(c, pc3060_switch, 5_gbps); });


  return t;
}

