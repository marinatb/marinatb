#include "blueprints.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using namespace marina;
using namespace pipes;

Blueprint marina::mars()
{
  Blueprint m{"mars"};

  //phobos network -------------------------------------------------------------
  auto phobos =
  m.network("phobos")
    .capacity(3_gbps)
    .latency(27_ms);
  
  auto c =
  m.computer("wiley")
    .os("ubuntu-desktop-15.10")
    .memory(4_gb)
    .cores(4)
    .add_ifx("ifx0", 1_gbps);

  m.connect(c.ifx("ifx0"), phobos);

  c =
  m.computer("cortana")
    .os("windows-10")
    .memory(4_gb)
    .cores(4)
    .add_ifx("ifx0", 1_gbps);
  
  m.connect(c.ifx("ifx0"), phobos);

  //deimos network -------------------------------------------------------------
  auto deimos =
  m.network("deimos")
    .capacity(1_gbps)
    .latency(17_ms);

  auto dcomp = [&m,deimos](string name, string os, size_t cores=2, 
      Memory mem=4_gb)
  {
    auto c =
    m.computer(name).os(os).cores(cores).memory(mem).add_ifx("ifx0", 1_gbps);
    m.connect(c.ifx("ifx0"), deimos);
  };

  dcomp("wolf", "ubuntu-server-15.10", 12, 16_gb);
  dcomp("longhorn", "windows-7");
  dcomp("penny", "centos-7", 4, 8_gb);
  dcomp("blue", "windows-2012", 8, 16_gb);

  //mars network ---------------------------------------------------------------
  auto mars =
  m.network("mars")
    .capacity(100_mbps)
    .latency(13_ms);

  m.connect(phobos, mars);
  m.connect(deimos, mars);

  return m;
}
