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

/*
 *    local volatile runtime state
 */

//a map from network ids to virtual bridge numbers
unordered_map<string, size_t> netid_vbr_map;

//a map from mac addresses to virtual host port numbers
unordered_map<string, size_t> mac_vhost_map;

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



int main(int argc, char **argv)
{
  Glog::init("host-control");
  LOG(INFO) << "host-control starting";

  gflags::SetUsageMessage(
      "usage: host-control -pbr_mac <mac> -pbr_addr <addr>");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

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

  srv.run();
}

void execFail(const CmdResult & cr, string msg)
{
  LOG(ERROR) << msg;
  LOG(ERROR) << "exit code: " << cr.code;
  LOG(ERROR) << "output: " << cr.output;
  throw runtime_error{"launchVm failed"};
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

void launchVm(string img, size_t vnc_port, size_t cores, Memory mem, 
    size_t qmp_port, const Computer & c)
{
  //TODO more interfaces later
  Interface ifx = c.interfaces().begin()->second;

  string cmd = fmt::format(
    "qemu-system-x86_64 "
      "--enable-kvm "
      "-cpu IvyBridge -smp {cores},sockets=%1,cores=%1,threads=%1 "
      "-m {mem} -mem-path /dev/hugepates -mem-prealloc "
      "-object "
      " memory-backend-file,id=mem0,size={mem}M,mem-path=/dev/hugepages,share=on "
      "-numa node,memdev=mem0 -mem-prealloc "
      "-hda {img} "
      "-chardev socket,id=chr0,path=/var/run/openvswitch/mrtb-vbr-{mac} "
      "-netdev type=vhost-user,id=net0,chardev=chr0,vhostforce "
      "-device virtio-net-pci,mac={mac},netdev=net0 "
      "-vnc 0.0.0.0:{vnc},password "
      "-monitor stdio "
      "-qmp tcp:0.0.0.0:{qmp},server",
      fmt::arg("cores", cores),
      fmt::arg("mem", mem.megabytes()),
      fmt::arg("img", img),
      fmt::arg("mag", ifx.mac()),
      fmt::arg("vnc", vnc_port),
      fmt::arg("qmp", qmp_port)
  );

  CmdResult cr = exec(cmd);

  if(cr.code != 0)
  {
    LOG(ERROR) << "failed to create virtual machine";
    LOG(ERROR) << "exit code: " << cr.code;
    LOG(ERROR) << "output: " << cr.output;
    throw runtime_error{"launchVm failed"};
  }
}


void createNetworkBridge(const Network & n)
{
  //create a bridge for the network
  size_t net_id = netid_vbr_map.size();
  netid_vbr_map[n.guid()] = net_id;

  string br_id = fmt::format("mrtb-vbr-{}", net_id);
  
  LOG(INFO) << "creating net-bridge: " << n.name() << " -- " << br_id;

  string cmd = fmt::format(
    "ovs-vsctl add-br {id} -- set bridge {id} datapath_type=netdev", 
    fmt::arg("id", br_id)
  );

  CmdResult cr = exec(cmd);
  if(cr.code != 0)
  {
    LOG(ERROR) << "failed to create network bridge";
    LOG(ERROR) << "exit code: " << cr.code;
    LOG(ERROR) << "output: " << cr.output;
    throw runtime_error{"createNetworkBridge:create failed"};
  }

  //TODO xxx hardcode
  string remote_ip{"192.168.247.2"};

  //hook vxlan up to the bridge
  cmd = fmt::format(
    "ovs-vsctl add-port {id} vxlan-{gid} "
    "-- set Interface vxlan-{gid} type=vxlan "
    "options:remote_ip={ip}",
    fmt::arg("id", br_id),
    fmt::arg("gid", net_id),
    fmt::arg("ip", remote_ip)
  );

  cr = exec(cmd);
  if(cr.code != 0)
  {
    LOG(ERROR) << "failed to setup vxlan tunnel for network " << n.name();
    LOG(ERROR) << "exit code: " << cr.code;
    LOG(ERROR) << "output: " << cr.output;
    throw runtime_error{"createNetworkBridge:vxlan failed"};
  }

}

void createComputerPort(const Network & n, string id)
{
  string br_id = fmt::format("mrtb-vbr-{}", netid_vbr_map[n.guid()]);

  size_t vhost_id = mac_vhost_map.size();
  mac_vhost_map[n.guid()] = vhost_id;
  string po_id = fmt::format("mrtb-vhu-{}", vhost_id);

  LOG(INFO) << 
    fmt::format("creating network port %s on network %s",
                  id,
                  n.name());
 
  string cmd = fmt::format(
    "ovs-vsctl add-port {bid} {pid} -- set Interface {pid} type=dpdkvhostuser",
    fmt::arg("bid", br_id),
    fmt::arg("pid", po_id)
  );

  CmdResult cr = exec(cmd);

  if(cr.code != 0)
  {
    LOG(ERROR) << "createComputerPort failed";
    LOG(ERROR) << "exit code: " << cr.code;
    LOG(ERROR) << "output: " << cr.output;
    throw runtime_error{"createComputerPort failed"};
  }
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
          createComputerPort(n, nbr.id);
        }
      }
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
