#include "marina.hxx"

using namespace std;
using namespace marina;

BLUEPRINT
{
  Blueprint bp{"edge-build-test"};

  string[4] names = {"a", "b", "c", "d"};
  for(int i=0; i<4; ++i)
  {
    bp.computer(names[i],
      .os("ubunutu-server-xenial")
      .memory(1_gb)
      .cores(2)
      .disk(10_gb)
      .add_ifx("ifx0", 1_gbps);
  }
}
