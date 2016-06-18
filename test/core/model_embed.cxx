#include <iostream>
#include <algorithm>
#include "../catch.hpp"
#include "3p/pipes/pipes.hxx"
#include "test/models/blueprints/blueprints.hxx"
#include "test/models/topos/topologies.hxx"
#include "core/embed.hxx"

using std::string;
using std::cout;
using std::endl;
using std::to_string;
using std::vector;
using std::sort;
using std::lexicographical_compare;
using namespace marina;

using namespace pipes;

TEST_CASE("hello-marina", "[embed]")
{
   
  TestbedTopology t = minibed();
  EChart ec{t};
  Blueprint b = hello_marina();

  cout << ec.overview() << endl;

  EChart ec_ = embed(b, ec, t);

  cout << ec_.overview() << endl;

  EChart ec__ = unembed(b, ec_);
  
  cout << ec__.overview() << endl;
  
  /*
  TestbedTopology t = minibed();
  Blueprint b = hello_marina();
  EChart c;

  for(int i=0; i<2; ++i)
  {

    cout 
      << "begin"
      << "--" << endl
      << t.quickStatus() 
      << "--" << endl;

    auto t_embedded = embed(b, t, c);
    cout 
      << "embedded"
      << "--" << endl
      << t_embedded.quickStatus() 
      << "--" << endl;
    
    for(const Computer & c : b.computers())
    {
      cout << c.name() << " --> " << c.embedding().host << endl;
    }

    auto t_unembedded = unembed(b, t_embedded);
    cout 
      << "un-embedded"
      << "--" << endl
      << t_unembedded.quickStatus() 
      << "--" << endl;

    for(const Computer & c : b.computers())
    {
      cout << c.name() << " --> " << c.embedding().host << endl;
    }

    REQUIRE( t == t_unembedded );
  }
  */
}

TEST_CASE("mars", "[launch]")
{
  /*
  TestbedTopology t = deter2015();
  Blueprint m = mars();

  //uncomment to dump json representation
  //cout << m.json().dump(2) << endl;
  
  cout << "host computers: " << t.hosts().size() << endl 
       << "switches: " << t.switches().size() << endl << endl;
  
  EChart c;
  auto e = embed(m, t, c);

  cout << endl;

  auto in_service_hosts = 
    e.switches()
      | flatmap<vector>([&t](auto s)
        {
          return
          t.connectedHosts(s)
            | filter([](auto h){ return h.machines().empty(); });
        });

  auto in_service_hosts_ = e.hosts() 
    | filter([](auto h){ return h.machines().empty(); })
    | map<vector>([](auto x){ return x; });

  auto do_sort = [](auto & c)
  {
    sort(c.begin(), c.end(),
      [](const Host & a, const Host & b)
      {
        return
        lexicographical_compare(
            a.name().begin(), a.name().end(),
            b.name().begin(), b.name().end() );
      });
  };

  cout << "in-service hosts" << endl;
  do_sort(in_service_hosts);
  for(auto h : in_service_hosts) cout << h.name() << endl;

  cout << "in-service hosts __" << endl;
  do_sort(in_service_hosts_);
  for(auto h : in_service_hosts_) cout << h.name() << endl;

  cout << endl;

  REQUIRE(in_service_hosts.size() == in_service_hosts_.size());

  for(size_t i=0; i<in_service_hosts.size(); ++i)
    REQUIRE(
      in_service_hosts[i] == in_service_hosts_[i]
    );
  */

  /*
   * Example of iterating through switches
   *
    
      for(const auto & s : t.switches())
      {
        cout << "switch: " << s.name() << endl;
        for(const auto & c : s.connectedHosts())
        {
          cout << "  " << c.name() << endl;
        }
      }

  */

  //uncomment to dump json representation
  //cout << t.json().dump(2) << endl;
}
