#include "glog.hxx"

using namespace marina;
using std::string;

bool Glog::initialized{false};

void Glog::init(string service_name)
{
  google::InitGoogleLogging(service_name.c_str());
  google::InstallFailureSignalHandler();
  Glog::initialized = true;
}
