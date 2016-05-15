#include "blueprints.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using namespace pipes;
using namespace marina;

Blueprint marina::hello_marina()
{
  Blueprint bp{"hello-marina"};

  //lan ................................

  auto lan = 
  bp.network("lan")
    .capacity(1_gbps)
    .latency(5_ms);


  auto a =
  bp.computer("a")
    .os("ubuntu-server-16.10")
    .memory(4_gb)
    .cores(2)
    .add_ifx("ifx0", 1_gbps);

  bp.connect(a.ifx("ifx0"), lan);
  
  auto b =
  bp.computer("b")
    .os("ubuntu-server-16.10")
    .memory(4_gb)
    .cores(2)
    .add_ifx("ifx0", 1_gbps);

  bp.connect(b.ifx("ifx0"), lan);
  
  auto c =
  bp.computer("c")
    .os("ubuntu-server-16.10")
    .memory(4_gb)
    .cores(2)
    .add_ifx("ifx0", 1_gbps)
    .add_ifx("ifx1", 1_gbps)
    .add_ifx("ifx2", 1_gbps);

  bp.connect(c.ifx("ifx0"), lan);

  //wan ................................

  auto d =
  bp.computer("d")
    .os("ubuntu-server-16.10")
    .memory(4_gb)
    .cores(2)
    .add_ifx("ifx0", 1_gbps)
    .add_ifx("ifx1", 1_gbps);
  
  auto e =
  bp.computer("e")
    .os("ubuntu-server-16.10")
    .memory(4_gb)
    .cores(2)
    .add_ifx("ifx0", 1_gbps)
    .add_ifx("ifx1", 1_gbps);

  auto c_d =
  bp.network("c-d")
    .capacity(100_mbps)
    .latency(33_ms);

  bp.connect(c.ifx("ifx1"), c_d);
  bp.connect(d.ifx("ifx0"), c_d);

  auto c_e =
  bp.network("c-e")
    .capacity(100_mbps)
    .latency(7_ms);

  bp.connect(c.ifx("ifx2"), c_e);
  bp.connect(e.ifx("ifx0"), c_e);

  auto d_e =
  bp.network("d-e")
    .capacity(100_mbps)
    .latency(14_ms);

  bp.connect(d.ifx("ifx1"), d_e);
  bp.connect(e.ifx("ifx1"), d_e);

  return bp;
}
