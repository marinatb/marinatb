#include <iostream>
#include "core/blueprint.hxx"
#include "../catch.hpp"

using std::cout;
using std::endl;
using namespace marina;

TEST_CASE("ipv4-addr", "[net-ipv4]")
{
  IpV4Address a{"192.168.0.1", 24};

  REQUIRE( a.cidr() == "192.168.0.1/24" );

  for(int i=0; i<10; ++i)
  {
    cout << a.addrStr() << endl;
    a = a + 50;
  }
  
  for(int i=0; i<10; ++i)
  {
    a = a - 50;
    cout << a.addrStr() << endl;
  }
  for(int i=0; i<10; ++i)
  {
    cout << a.addrStr() << endl;
    a++;
  }
  for(int i=0; i<10; ++i)
  {
    a--;
    cout << a.addrStr() << endl;
  }

  REQUIRE( a.netZero() == false );
  a = IpV4Address{"10.10.47.0", 24};
  REQUIRE( a.netZero() == true );
}
