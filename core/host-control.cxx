/*
 * The marina testbed host-control service implementation
 */

#include <cstdlib>
#include <unordered_map>
#include <fmt/format.h>
#include <gflags/gflags.h>
#include "common/net/http_server.hxx"
#include "core/blueprint.hxx"
#include "core/util.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using std::to_string;
using std::unordered_map;
using std::unique_ptr;
using std::vector;
using std::runtime_error;
using std::out_of_range;
using std::exception;
using wangle::SSLContextConfig;
using namespace pipes;
using namespace marina;

http::Response construct(Json);
http::Response destruct(Json);
http::Response info(Json);
http::Response list(Json);

void execFail(const CmdResult &, string);
void initOvs();
void initQemuKvm();

/*
 *    local volatile runtime state
 */

/*    bridge info   */
LinearIdCacheMap<string, size_t> bridgeId, vhostId, qkId;

/*
 *    command line flags
 */
DEFINE_string(
  pbr_mac, 
  "00:00:00:00:0B:AD", 
  "the physical dpdk bridge mac address to use"
);

DEFINE_string(
  pbr_addr,
  "192.168.247.1/24",
  "the physical dpdk bridge ip address to use"
);

DEFINE_string(
  remote_addr,
  "192.168.247.2",
  "remote vxlan address"
);

int main(int argc, char **argv)
{
  Glog::init("host-control");
  LOG(INFO) << "host-control starting";

  gflags::SetUsageMessage(
      "usage: host-control -pbr_mac <mac> -pbr_addr <addr>");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  initQemuKvm();
  initOvs();
  
  SSLContextConfig sslc;
  sslc.setCertificate(
      "/marina/cert.pem",
      "/marina/key.pem",
      "" //no password on cert
  );
  
  HttpsServer srv("0.0.0.0", 443, sslc);
  
  srv.onPost("/construct", jsonIn(construct));
  srv.onPost("/destruct", jsonIn(destruct));
  srv.onPost("/info", jsonIn(info));
  srv.onPost("/list", jsonIn(list));

  LOG(INFO) << "ready";

  srv.run();
}

void execFail(const CmdResult & cr, string msg)
{
  LOG(ERROR) << msg;
  LOG(ERROR) << "exit code: " << cr.code;
  LOG(ERROR) << "output: " << cr.output;
  throw runtime_error{msg};
}

void initOvs()
{
  LOG(INFO) << "wiping any old ovs config";
  
  exec("service openvswitch-switch stop");
  exec("rm -rf /var/log/openvswitch/* /etc/openvswitch/conf.db");
  CmdResult cr = exec("service openvswitch-switch start");
  if(cr.code != 0) execFail(cr, "failed to clean start openvswitch");

  // create the physical bridge
  LOG(INFO) << "setting up physical bridge";
  cr = exec(
    fmt::format(
      "ovs-vsctl add-br mrtb-pbr "
      "-- set bridge mrtb-pbr datapath_type=netdev "
      "other_config:hwaddr={pmac}",
      fmt::arg("pmac", FLAGS_pbr_mac) 
    )
  );
  if(cr.code != 0) execFail(cr, "failed to create physical bridge");

  // hook up dpdk
  cr = exec(
    "ovs-vsctl add-port mrtb-pbr dpdk0 -- set Interface dpdk0 type=dpdk"
  );
  if(cr.code != 0) 
    execFail(cr, "failed to add dpdk interface to physical bridge");

  // give the bridge an address
  cr = exec(
    fmt::format(
      "ip addr add {paddr} dev mrtb-pbr",
      fmt::arg("paddr", FLAGS_pbr_addr)
    )
  );
  if(cr.code != 0) 
    execFail(cr, "failed to give physical bridge an ip address");

  // bring physical bridge up
  cr = exec("ip link set mrtb-pbr up");
  if(cr.code != 0) execFail(cr, "failed to bring physical bridge up");

  // flush iptables
  cr = exec("iptables -F");
  if(cr.code != 0) execFail(cr, "failed to flush iptables");

  LOG(INFO) << "ovs ready";

}

void initQemuKvm()
{
  LOG(INFO) << "clobbering any existing qemu-system instances";

  CmdResult cr = exec("pkill qemu-system");
  //if(cr.code != 0) execFail(cr, "failed to kill existing qemu instances");

  LOG(INFO) << "qemu-kvm ready";
}

void makeDiskImage(string name, Memory size)
{
  string sz;
  if(size.unit == Memory::Unit::B) sz = "";
  else if(size.unit == Memory::Unit::KB) sz = "k";
  else if(size.unit == Memory::Unit::MB) sz = "M";
  else if(size.unit == Memory::Unit::GB) sz = "G";
  else if(size.unit == Memory::Unit::TB) sz = "T";

  string cmd = 
    "qemu-img create -f qcow2 " + name + ".qcow2 " + to_string(size.size) + sz;

  system(cmd.c_str());
}

void launchVm(const Computer & c)
{
  size_t qk_id = qkId.create(c.interfaces().begin()->second.mac());

  //create qemu interfaces
  size_t k{0};
  string netblk;
  for(const auto & i : c.interfaces())
  {
    Interface ifx = i.second;
    
    size_t vhid = vhostId.get(ifx.mac());

    string blk =
      fmt::format(
        "-chardev socket,id=chr{k},path=/var/run/openvswitch/mrtb-vhu-{vhid} "
        "-netdev type=vhost-user,id=net{k},chardev=chr{k},vhostforce "
        "-device virtio-net-pci,mac={mac},netdev=net{k} ",
        fmt::arg("vhid", vhid),
        fmt::arg("mac", ifx.mac()),
        fmt::arg("k", k)
      );

    netblk += blk;

    ++k;
  }

  //TODO come up with an actual image naming/storage mech
  string img_src = fmt::format("/space/images/std/{}.qcow2", c.os());
  string img = fmt::format("/space/images/run/{}-{}.qcow2", c.os(), qk_id);

  //string cmd = fmt::format("cp {} {}", img_src, img);
  string cmd = fmt::format("qemu-img create -f qcow2 -o backing_file={src} {tgt}",
      fmt::arg("src", img_src),
      fmt::arg("tgt", img)
  );
  CmdResult cr = exec(cmd);
  if(cr.code != 0) execFail(cr, "failed to create disk image for vm");

  LOG(INFO) << fmt::format("{name}.qemu = /tmp/mrtb-qk{id}-pid",
      fmt::arg("name", c.name()),
      fmt::arg("id", qk_id) 
    );

  cmd = fmt::format(
    "qemu-system-x86_64 "
      "--enable-kvm "
      "-cpu IvyBridge -smp {cores},sockets=1,cores={cores},threads=1 "
      "-m {mem} -mem-path /dev/hugepages -mem-prealloc "
      "-object "
      " memory-backend-file,id=mem0,size={mem}M,mem-path=/dev/hugepages,share=on "
      "-numa node,memdev=mem0 -mem-prealloc "
      "-hda {img} "
      "{netblk} "
      "-vnc 0.0.0.0:{qkid} "
      "-D /tmp/mrtb-qlog-{qkid} "
      "-daemonize "
      "-pidfile /tmp/mrtb-qk{qkid}-pid",
      fmt::arg("cores", c.cores()),
      fmt::arg("mem", c.memory().megabytes()),
      fmt::arg("img", img),
      fmt::arg("qkid", qk_id),
      fmt::arg("netblk", netblk)
  );

  cr = exec(cmd);

  if(cr.code != 0) execFail(cr, "failed to create virtual machine");
}


void createNetworkBridge(const Network & n)
{
  //create a bridge for the network
  size_t net_id = bridgeId.create(n.guid());
  string br_id = fmt::format("mrtb-vbr-{}", net_id);

  string cmd = fmt::format(
    "ovs-vsctl add-br {id} -- set bridge {id} datapath_type=netdev", 
    fmt::arg("id", br_id)
  );
  LOG(INFO) << fmt::format("{}.net-bridge = {}", n.name(), br_id);

  CmdResult cr = exec(cmd);
  if(cr.code != 0) execFail(cr, "failed to create network bridge");

  //TODO we only need to setup vxlan on the network if it has remote
  //elements (likely) and we should be figuring out the addresses of
  //the remotes with the embedding info
  //string remote_ip{"192.168.247.2"};
  string remote_ip = FLAGS_remote_addr;

  string vx_id = fmt::format("vxlan{}", net_id);

  //hook vxlan up to the bridge
  cmd = fmt::format(
    "ovs-vsctl add-port {id} {vxid} "
    "-- set Interface {vxid} type=vxlan "
    "options:remote_ip={ip} options:key={vni}",
    fmt::arg("id", br_id),
    fmt::arg("vxid", vx_id),
    fmt::arg("ip", remote_ip),
    fmt::arg("vni", n.einfo().vni)
  );

  cr = exec(cmd);
  if(cr.code != 0) execFail(cr, "failed to setup vxlan for network " + n.name());

  LOG(INFO) << fmt::format("{}.vxlan = {}", br_id, vx_id);
}

void createComputerPort(const Network & n, string id)
{
  string br_id = fmt::format("mrtb-vbr-{}", bridgeId.get(n.guid()));

  size_t vhost_id = vhostId.create(id);
  string po_id = fmt::format("mrtb-vhu-{}", vhost_id);

  LOG(INFO) << fmt::format("{}.uplink = {}", id, po_id);
 
  string cmd = fmt::format(
    "ovs-vsctl add-port {bid} {pid} -- set Interface {pid} type=dpdkvhostuser",
    fmt::arg("bid", br_id),
    fmt::arg("pid", po_id)
  );

  LOG(INFO) << fmt::format("{} -- {}", br_id, po_id);

  CmdResult cr = exec(cmd);

  if(cr.code != 0) execFail(cr, "failed to create computer port");
}

http::Response construct(Json j)
{
  LOG(INFO) << "construct request";
  LOG(INFO) << j.dump(2);

  try
  {
    auto bp = Blueprint::fromJson(j); 

    LOG(INFO) 
      << "materializaing " 
      << bp.computers().size()  << " computers"
      << " across "
      << bp.networks().size() << " networks";

    google::FlushLogFiles(0);

    for(const Network & n : bp.networks()) 
    {
      createNetworkBridge(n);
      for(const Neighbor & nbr : n.connections())
      {
        if(nbr.kind == Neighbor::Kind::Computer)
        {
          try
          { 
            bp.getComputerByMac(nbr.id);
            createComputerPort(n, nbr.id);
          }
          catch(out_of_range &) 
          { 
            // expected that some comps not local
            LOG(INFO) << fmt::format(
              "skipping interface generation for remote network connection "
              "{} -- {}",
              n.name(), nbr.id
            );
          }
        }
      }
    }
    for(const Computer & c : bp.computers())
    {
      launchVm(c);
    }

    Json r;
    r["status"] = "ok";
    return http::Response{ http::Status::OK(), r.dump() };
  }
  catch(out_of_range &) { return badRequest("construct", j); }
  catch(exception &e) { return unexpectedFailure("construct", j, e); }

}

http::Response info(Json)
{
  throw runtime_error{"not implemented"};
}

http::Response destruct(Json)
{
  throw runtime_error{"not implemented"};
}

http::Response list(Json)
{
  throw runtime_error{"not implemented"};
}
