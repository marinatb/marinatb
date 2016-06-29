#include "marina.hxx"

using namespace std;
using namespace marina;

BLUEPRINT
{
  Blueprint bp{"hello-marina"};

  auto comp = [&bp](string name)
  {
    return
    bp.computer(name)
      .os("ubuntu-server-xenial")
      .memory(1_gb)
      .cores(2)
      .disk(10_gb)
      .add_ifx("ifx0", 1_gbps);
  };

  //lan ...........................................

  auto lan = 
  bp.network("lan")
    .capacity(1_gbps)
    .latency(5_ms)
    .ipv4("10.10.47.0", 24);


  auto a = comp("a"),
       b = comp("b"),
       c = comp("c")
            .add_ifx("ifx1", 1_gbps)
            .add_ifx("ifx2", 1_gbps);

  bp.connect({a, a.ifx("ifx0")}, lan);
  bp.connect({b, b.ifx("ifx0")}, lan);
  bp.connect({c, c.ifx("ifx0")}, lan);

  //wan ..........................................

  auto d = comp("d").add_ifx("ifx1", 1_gbps),
       e = comp("e").add_ifx("ifx1", 1_gbps);

  auto 
    c_d = bp.network("c-d")
            .capacity(100_mbps)
            .latency(33_ms)
            .ipv4("107.7.13.0", 16),

    c_e = bp.network("c-e")
            .capacity(100_mbps)
            .latency(7_ms)
            .ipv4("97.3.6.0", 16),

    d_e = bp.network("d-e")
            .capacity(100_mbps)
            .latency(14_ms)
            .ipv4("104.7.31.0", 16);


  bp.connect({c, c.ifx("ifx1")}, c_d);
  bp.connect({d, d.ifx("ifx0")}, c_d);

  bp.connect({c, c.ifx("ifx2")}, c_e);
  bp.connect({e, e.ifx("ifx0")}, c_e);

  bp.connect({d, d.ifx("ifx1")}, d_e);
  bp.connect({e, e.ifx("ifx1")}, d_e);

  return bp;
}

