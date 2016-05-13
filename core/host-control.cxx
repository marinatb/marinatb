/*
 * The marina testbed host-control service implementation
 */

#include <cstdlib>
#include "common/net/http_server.hxx"
#include "core/blueprint.hxx"
#include "core/util.hxx"
#include "3p/pipes/pipes.hxx"

using std::string;
using std::to_string;
using std::runtime_error;
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

  LOG(INFO) << "node control starting";
  
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
  string cmd = 
    "qemu-img create -f qcow2 " + name + ".qcow2 " + to_string(size);

  system(cmd.c_str());
}

void launchVm(string img, size_t vnc_port, size_t cores, Memory mem, 
    size_t qmp_port)
{
  string cmd = 
    "qemu-system-x86_64" 
    " --enable-kvm -smp " + to_string(cores) +
    " -m " + to_string(mem.megabytes()) + 
    " -vnc 0.0.0.0:" + to_string(vnc_port) + ",password" +
    " -monitor stdio" + 
    " -qmp tcp:0.0.0.0:" + to_string(qmp_port) + ",server" +
    " " + img;

  system(cmd.c_str());
}

http::Response construct(Json)
{
  LOG(INFO) << "construct request";

  //extract request parameters
  vector<Json> computers_json;
  vector<Json> networks_json;
  try
  {
    computers_json = j.at("computers");
    networks_json = j.at("networks");

    vector<Computer> computers = 
      computers_json 
      | map([](const Json &x){ return Computer::fromJson(x); };
  }

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
