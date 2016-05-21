/*
 * The marina testbed host-control service implementation
 */

#include <cstdlib>
#include <fmt/format.h>
#include "common/net/http_server.hxx"
#include "core/blueprint.hxx"
#include "core/util.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using std::to_string;
using std::unique_ptr;
using std::runtime_error;
using std::vector;
using std::out_of_range;
using wangle::SSLContextConfig;
using namespace pipes;
using namespace marina;

http::Response construct(Json);
http::Response destruct(Json);
http::Response info(Json);
http::Response list(Json);

int main()
{
  Glog::init("host-control");

  LOG(INFO) << "host-control starting";
  
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
      "-cpu IvyBridge -smp %zu,sockets=%zu,cores=%zu,threads=%zu "
      "-m %zu -mem-path /dev/hugepates -mem-prealloc "
      "-object "
      " memory-backend-file,id=mem0,size=%zuM,mem-path=/dev/hugepages,share=on "
      "-numa node,memdev=mem0 -mem-prealloc "
      "-hda %s "
      "-chardev socket,id=chr0,path=/var/run/openvswitch/mrtb-vbr-%s "
      "-netdev type=vhost-user,id=net0,chardev=chr0,vhostforce "
      "-device virtio-net-pci,mac=%s,netdev=net0 "
      "-vnc 0.0.0.0:%zu,password "
      "-monitor stdio "
      "-qmp tcp:0.0.0.0:%zu,server",
      cores, 1, 1, 1,
      mem.megabytes(),
      mem.megabytes(),
      img,
      ifx.mac(),
      ifx.mac(),
      vnc_port,
      qmp_port
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
  string br_id = fmt::format("mrtb-vbr-%s", n.guid());
  
  LOG(INFO) << "creating net-bridge: " << n.name() << " -- " << br_id;

  string cmd = fmt::format(
    "ovs-vsctl add-br %s -- set bridge %s datapath_type=netdev", 
    br_id, 
    br_id
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
    "ovs-vsctl add-port %s vxlan0 -- set Interface vxlan0 type=vxlan "
    "options:remote_ip=%s",
    br_id,
    remote_ip
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
  string br_id = fmt::format("mrtb-vbr-%s", n.guid());
  string po_id = fmt::format("mrtb-vhu-%s", id);

  LOG(INFO) << 
    fmt::format("creating network port %s on network %s",
                  id,
                  n.name());
 
  string cmd = fmt::format(
    "ovs-vsctl add-port %s %s -- set Interface %s type=dpdkvhostuser",
    br_id,
    po_id,
    po_id
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

  //extract request parameters
  //vector<Json> computers_json;
  //vector<Json> networks_json;
  try
  {
    //computers_json = j.at("computers").get<vector<Json>>();
    //networks_json = j.at("networks").get<vector<Json>>();
    auto bp = Blueprint::fromJson(j); 

    /*
    vector<Computer> computers = 
      computers_json
      | map([](const Json &x){ return Computer::fromJson(x); });

    vector<Network> networks = 
      networks_json
      | map([](const Json &x){ return Network::fromJson(x); });
      */

    LOG(INFO) 
      << "materializaing " 
      << bp.computers().size()  << " computers"
      << " across "
      << bp.networks().size() << " networks";

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
  catch(out_of_range &) { return badRequest("save", j); }

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
