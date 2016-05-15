#include "blueprints.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using namespace pipes;
using namespace marina;

Blueprint marina::hello_marina()
{
  Blueprint bp{"hello-marina"};

  auto comp = [&bp](string name)
  {
    return
    bp.computer(name)
      .os("ubuntu-server-16.10")
      .memory(4_gb)
      .cores(2)
      .disk(10_gb)
      .add_ifx("ifx0", 1_gbps);
  };

  //lan ...........................................

  auto lan = 
  bp.network("lan")
    .capacity(1_gbps)
    .latency(5_ms);


  auto a = comp("a"),
       b = comp("b"),
       c = comp("c")
            .add_ifx("ifx1", 1_gbps)
            .add_ifx("ifx2", 1_gbps);

  bp.connect(a.ifx("ifx0"), lan);
  bp.connect(b.ifx("ifx0"), lan);
  bp.connect(c.ifx("ifx0"), lan);

  //wan ..........................................

  auto d = comp("d").add_ifx("ifx1", 1_gbps),
       e = comp("e").add_ifx("ifx1", 1_gbps);

  auto 
    c_d = bp.network("c-d")
            .capacity(100_mbps)
            .latency(33_ms),

    c_e = bp.network("c-e")
            .capacity(100_mbps)
            .latency(7_ms),

    d_e = bp.network("d-e")
            .capacity(100_mbps)
            .latency(14_ms);


  bp.connect(c.ifx("ifx1"), c_d);
  bp.connect(d.ifx("ifx0"), c_d);


  bp.connect(c.ifx("ifx2"), c_e);
  bp.connect(e.ifx("ifx0"), c_e);


  bp.connect(d.ifx("ifx1"), d_e);
  bp.connect(e.ifx("ifx1"), d_e);

  return bp;
}
