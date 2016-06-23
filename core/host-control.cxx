/*
 * The marina testbed host-control service implementation
 */

#include <cstdlib>
#include <thread>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include <fmt/format.h>
#include <gflags/gflags.h>
#include "3p/pipes/pipes.hxx"
#include "common/net/http_server.hxx"
#include "core/blueprint.hxx"
#include "core/util.hxx"
#include "core/db.hxx"

using std::string;
using std::to_string;
using std::unordered_map;
using std::unique_ptr;
using std::vector;
using std::ofstream;
using std::endl;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::defer_lock_t;
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

/*    
 *    bridge info
 *    +++++++++++
 *
 *    these data structures are thread safe
 *
 */

LinearIdCacheMap<Uuid, size_t, UuidHash, UuidCmp> bridgeId;
LinearIdCacheMap<string, size_t> qkId, vhostId;

/*
 * a map to keep track of the blueprints that are live on this host
 */

unordered_map<Uuid, Blueprint, UuidHash, UuidCmp> live_blueprints;
mutex lb_mtx;
unique_lock<mutex> lb_lk{lb_mtx, defer_lock_t{}};

unique_ptr<DB> db{nullptr};

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
  
  db.reset(new DB{"postgresql://murphy:muffins@db"});
  
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
  exec("rm -rf /var/run/openvswitch/mrtb*");
  CmdResult cr = exec("service openvswitch-switch start");
  if(cr.code != 0) execFail(cr, "failed to clean start openvswitch");

  // create the physical experiment bridge
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
  LOG(INFO) << "qemu-kvm ready";
}

inline string xpdir(const Blueprint & bp)
{
  return fmt::format("/space/xp/{}", bp.id().str());
}

void plopLinuxConfig(const Computer & c, string dst, string img)
{
  //create network init script for linux
  string initscript = fmt::format("{}/{}-netup", dst, c.name());
  ofstream ofs{ initscript };

  ofs << "#!/bin/bash"
      << endl
      << endl;
  
  ofs << "ldconfig" 
      << endl
      << endl;

  ofs << "m2i=mac2ifname"
      << endl
      << endl;

  ofs << "hostname " << c.name()
      << endl
      << endl;

  for(const auto & i : c.interfaces())
  {
    Interface ifx = i.second;
    string dev_name = fmt::format("`$m2i {}`", ifx.mac());

    if(ifx.name() == "cifx")
    {
      ofs << fmt::format("# control interface", dev_name) << endl;
      ofs << fmt::format("ip link set up dev {}", dev_name) << endl;
      ofs << fmt::format("dhclient {}", dev_name) << endl;
      ofs << endl;
    }
    else
    {
      string addr = ifx.einfo().ipaddr_v4.cidr();

      ofs << fmt::format("# {}", ifx.name()) << endl;
      ofs << fmt::format("ip addr add {} dev {}", addr, dev_name) << endl;
      ofs << fmt::format("ip link set up dev {}", dev_name) << endl;
      ofs << endl;
    }
  }
  ofs.close();
  exec(fmt::format("chmod +x {}", initscript));

  exec("modprobe nbd max_part=8");

  //copy the network init script to the guest drive image
  //unmount and disconnect anything that is already hooked up
  exec("umount /space/tmount");
  exec("qemu-nbd --disconnect /dev/nbd0"); 
  CmdResult cr = exec(fmt::format("qemu-nbd --connect=/dev/nbd0 {}", img));
  if(cr.code != 0) 
    execFail(cr, "failed to create network boodtdisk for qeumu image " + img);

  cr = exec("mount /dev/nbd0p1 /space/tmount");
  if(cr.code != 0)
  {
    exec("qemu-nbd --disconnect /dev/nbd0");
    execFail(cr, "failed to mount qemu network boot disk partition 1");
  }

  cr = exec("mkdir -p /space/tmount/marina");
  if(cr.code != 0)
  {
    exec("umount /space/tmount");
    exec("qemu-nbd --disconnect /dev/nbd0");
    execFail(cr, "failed to create marina root dir on node {}" + c.name());
  }

  cr = exec(fmt::format("cp {} {}", initscript, "/space/tmount/marina/init"));  
  if(cr.code != 0)
  {
    exec("umount /space/tmount");
    exec("qemu-nbd --disconnect /dev/nbd0");
    execFail(cr, "failed to plop initscript into " + c.name());
  }

  cr = exec("cp /home/murphy/host-pkg/libs/* /space/tmount/usr/local/lib/");
  if(cr.code != 0)
  {
    exec("umount /space/tmount");
    exec("qemu-nbd --disconnect /dev/nbd0");
    execFail(cr, "failed to plop libs into " + c.name());
  }
  
  cr = exec("cp /home/murphy/host-pkg/bin/* /space/tmount/usr/local/bin/");
  if(cr.code != 0)
  {
    exec("umount /space/tmount");
    exec("qemu-nbd --disconnect /dev/nbd0");
    execFail(cr, "failed to plop bins into " + c.name());
  }
  
  ofs = ofstream{"/space/tmount/etc/ld.so.conf.d/local.conf"};
  ofs << "/usr/local/lib" << endl;
  ofs.close();

  string svc{"/space/tmount/lib/systemd/system/marina.service"};
  ofs = ofstream{svc};

  ofs << "[Unit]" << endl
      << "Description=Init this computer as a part of a marinatb experiment" 
      << endl
      << endl

      << "[Service]" << endl
      << "Type=oneshot" << endl
      << "ExecStart=/marina/init" << endl
      << endl

      << "[Install]" << endl
      << "WantedBy=multi-user.target" << endl
      << endl;

  ofs.close();
  
  string svclink{
    "/space/tmount/etc/systemd/system/multi-user.target.wants/marina.service"};

  exec(fmt::format("ln -s /lib/systemd/system/marina.service {}", svclink));
    
  exec("umount /space/tmount");
  exec("qemu-nbd --disconnect /dev/nbd0");

}

void launchVm(const Computer & c, const Blueprint & bp)
{
  size_t qk_id = qkId.create(c.interfaces().at("cifx").mac());

  string arch{"IvyBridge"};

  //create qemu interfaces
  size_t k{0};
  string netblk;
  string ctlmac;
  for(const auto & i : c.interfaces())
  {
    Interface ifx = i.second;

    if(i.first == "cifx") 
    {
      ctlmac = ifx.mac();
      continue;
    }
    
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

  //create the disk image
  string img_src = fmt::format("/space/images/std/{}.qcow2", c.os());
  string img = fmt::format("{}/{}.qcow2", xpdir(bp), c.name());
  string cmd = fmt::format(
      "qemu-img create -f qcow2 -o backing_file={src} {tgt}",
      fmt::arg("src", img_src),
      fmt::arg("tgt", img)
  );
  CmdResult cr = exec(cmd);
  if(cr.code != 0) execFail(cr, "failed to create disk image for vm");

  LOG(INFO) << fmt::format("{name}.qemu = /tmp/mrtb-qk{id}-pid",
      fmt::arg("name", c.name()),
      fmt::arg("id", qk_id) 
    );
  
  if(isLinux(c))
  {
    plopLinuxConfig(c, xpdir(bp), img);
  }
  else
  {
    throw "tomatoes";
  }

  cmd = fmt::format(
    "qemu-system-x86_64 "
      "--enable-kvm "
      "-cpu {arch} -smp {cores},sockets=1,cores={cores},threads=1 "
      "-m {mem} -mem-path /dev/hugepages -mem-prealloc "
      "-object "
      " memory-backend-file,id=mem0,size={mem}M,mem-path=/dev/hugepages,share=on "
      "-numa node,memdev=mem0 -mem-prealloc "
      "-hda {img} "
      "{netblk} "
      "-vnc 0.0.0.0:{qkid} "
      "-D /{xpdir}/{name}-qlog "
      "-net nic,vlan={qkid},macaddr={ctlmac} "
      "-net user,vlan={qkid},hostfwd=tcp::220{qkid}-:22 "
      "-daemonize "
      "-pidfile /{xpdir}/{name}-qpid",
      fmt::arg("arch", arch),
      fmt::arg("cores", c.cores()),
      fmt::arg("mem", c.memory().megabytes()),
      fmt::arg("img", img),
      fmt::arg("ctlmac", ctlmac),
      fmt::arg("qkid", qk_id),
      fmt::arg("netblk", netblk),
      fmt::arg("xpdir", xpdir(bp)),
      fmt::arg("name", c.name())
  );

  cr = exec(cmd);

  if(cr.code != 0) execFail(cr, "failed to create virtual machine");
}


void createNetworkBridge(const Network & n)
{
  //create a bridge for the network
  size_t net_id = bridgeId.create(n.id());

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

  //hook experiment-vxlan up to the network-bridge
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

void createComputerPort(const Network & n, string mac)
{
  string br_id = fmt::format("mrtb-vbr-{}", bridgeId.get(n.id()));

  size_t vhost_id = vhostId.create(mac);
  string po_id = fmt::format("mrtb-vhu-{}", vhost_id);

  LOG(INFO) << fmt::format("{}.uplink = {}", mac, po_id);
 
  string cmd = fmt::format(
    "ovs-vsctl add-port {bid} {pid} -- set Interface {pid} type=dpdkvhostuser",
    fmt::arg("bid", br_id),
    fmt::arg("pid", po_id)
  );

  LOG(INFO) << fmt::format("{} -- {}", br_id, po_id);

  CmdResult cr = exec(cmd);

  if(cr.code != 0) execFail(cr, "failed to create computer port");
}

void initXpDir(const Blueprint & bp)
{
  exec(fmt::format("rm -rf {}", xpdir(bp)));
  CmdResult cr = exec(fmt::format("mkdir -p {}", xpdir(bp)));
  if(cr.code != 0) execFail(cr, "failed to create experiment directory");

  {
    ofstream ofs{fmt::format("{}/blueprint", xpdir(bp))};
    ofs << bp.json().dump(2);
    ofs.close();
  }
}

void launchNetworks(const Blueprint & bp)
{
  for(const auto & p : bp.networks()) 
  {
    const Network & n = p.second;
    createNetworkBridge(n);
    for(auto & p : bp.connectedComputers(n))
    {
      createComputerPort(n, p.second.mac());
    }
  }
}

void launchComputers(Blueprint & bp)
{
  //TODO with EChart
  /*
  using LS = Computer::EmbeddingInfo::LaunchState;
  for(const auto & c : bp.computers())
  {
    c.second.embedding().launch_state = LS::Queued;
  }
  */

  //TODO need to do this more granularly, e.g. just update the launch state
  //because this is an overwriting race between the host controllers
  //db->saveMaterialization(bp.project(), bp.name(), bp.json());

  for(const auto & c : bp.computers())
  {
    //TODO with EChart
    //c.second.embedding().launch_state = LS::Launching;
    launchVm(c.second, bp);
    //TODO with EChart
    //c.second.embedding().launch_state = LS::Up;
  }
}

http::Response construct(Json j)
{
  LOG(INFO) << "construct request";
  LOG(INFO) << j.dump(2);

  thread t{[j](){
    try
    {
      auto bp = Blueprint::fromJson(j); 

      lb_lk.lock();
      live_blueprints.insert_or_assign(bp.id(), bp);
      lb_lk.unlock();

      LOG(INFO) 
        << "materializaing " 
        << bp.computers().size()  << " computers"
        << " across "
        << bp.networks().size() << " networks";

      initXpDir(bp);
      launchNetworks(bp);
      launchComputers(bp);

      LOG(INFO) << fmt::format("{name}({id}) has been materialized",
            fmt::arg("name", bp.name()),
            fmt::arg("id", bp.id().str())
          );

    }
    catch(out_of_range &e) { badRequest("construct", j, e); }
    catch(exception &e) { unexpectedFailure("construct", j, e); }
  }};

  //TODO perhaps the better thing to do here would be to create a
  //thread pointer and put it in a map that is keyed on the blueprint
  //id that is being materialized, that way we could support things
  //like cancellation and possibly better progress reporting
  t.detach();

  Json r;
  r["status"] = "materializing";
  return http::Response{ http::Status::OK(), r.dump() };

}

void delXpDir(const Blueprint &bp)
{
  exec(fmt::format("rm -rf {}", xpdir(bp)));
}

void terminateComputers(const Blueprint & bp)
{
  for(const auto & c : bp.computers())
  {
    string cmd = 
      fmt::format("kill `cat /{xpdir}/{name}-qpid`",
        fmt::arg("xpdir", xpdir(bp)),
        fmt::arg("name", c.second.name())
      );
    CmdResult cr = exec(cmd);
    if(cr.code != 0) 
      execFail(cr, 
          fmt::format("failed to terminate computer {}/{}", 
            bp.id().str(), 
            c.second.name())
      );

    qkId.erase(c.second.interfaces().at("cifx").mac());
   
  }
}

void terminateNetworks(const Blueprint & bp)
{
  for(const auto & p : bp.networks())
  {
    const Network & n = p.second;
    string br_id = fmt::format("mrtb-vbr-{}", bridgeId.get(n.id()));
    string cmd = fmt::format("ovs-vsctl del-br {}", br_id);
    CmdResult cr = exec(cmd);
    if(cr.code != 0) execFail(cr, "failed to terminate network bridge "+br_id);

    bridgeId.erase(n.id());
    
    for(auto & p : bp.connectedComputers(n))
    {
      vhostId.erase(p.second.mac());
    }
  }
}

http::Response destruct(Json j)
{
  LOG(INFO) << "destruct request";
  LOG(INFO) << j.dump(2);

  try
  {
    auto bp = Blueprint::fromJson(j);

    terminateComputers(bp);
    terminateNetworks(bp);
    delXpDir(bp);
    
    LOG(INFO) << fmt::format("{name}({guid}) has been dematerialized",
          fmt::arg("name", bp.name()),
          fmt::arg("guid", bp.id().str())
        );

    Json r;
    r["status"] = "ok";
    return http::Response{ http::Status::OK(), r.dump() };
  }
  catch(out_of_range &e) { return badRequest("destruct", j, e); }
  catch(exception &e) { return unexpectedFailure("destruct", j, e); }

  throw runtime_error{"not implemented"};
}

http::Response info(Json j)
{
  LOG(INFO) << "info request";

  //extract request parameters
  Uuid bpid;
  try
  {
    bpid = Uuid::fromJson(extract(j, "bpid", "info-request"));
  }
  catch(out_of_range &e) { return badRequest("info", j, e); }

  try
  {
    return http::Response{ 
      http::Status::OK(), 
      live_blueprints.at(bpid).json().dump(2)
    };
  }
  catch(out_of_range)
  {
    Json msg;
    msg["error"] = "bpid "+bpid.str()+" is not being materialized here";

    return http::Response{
      http::Status::BadRequest(),
      msg.dump(2)
    };
  }
  //something we did not plan for, but keep the service going none the less
  catch(exception &e) { return unexpectedFailure("list", j, e); }
}

http::Response list(Json)
{
  throw runtime_error{"not implemented"};
}
