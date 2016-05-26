/*******************************************************************************
 *
 *  mac_2_ifname: given a mac address return the corresponding interface name
 *
 *  usage: mac2ifname <mac>
 *
 *  build: c++ -std=c++14 mac2ifname.cxx -o mac2ifname
 *
 ******************************************************************************/

#include "core/util.hxx"

using std::cerr;
using std::cout;
using std::endl;
using std::out_of_range;
using std::string;
using namespace marina;

int main(int argc, char **argv)
{
  if(argc != 2)
  {
    cerr << "usage: mac2ifname <mac>" << endl;
    return 1;
  }
  try { cout << mac_2_ifname(string{argv[1]}) << endl; }
  catch(out_of_range &e) 
  { 
    cerr << e.what() << endl;
    return 1;
  }
}
