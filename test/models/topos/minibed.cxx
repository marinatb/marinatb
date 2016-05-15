#include "topologies.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using std::to_string;
using namespace marina;
using namespace pipes;
using namespace std::literals::string_literals;

TestbedTopology marina::minibed()
{
  TestbedTopology t{"minibed"};

  auto sw = t.sw("main-switch").backplane(250_tbps);
  range(0,2)
    | map([](int x) { return to_string(x); })
    | for_each([&sw, &t](string x) 
      {
        auto c = 
        t.host("mrtb"+x)
          .cores(24)
          .memory(24_gb)
          .disk(240_gb)
          .ifx(24_gbps);

        t.connect(c, sw, 24_gbps);
     });

  return t;
}
