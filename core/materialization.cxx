#include <stdexcept>
#include "core/materialization.hxx"

using namespace marina;
using std::string;
using std::vector;
using std::lock_guard;
using std::mutex;
using std::runtime_error;

// MzMap -----------------------------------------------------------------------

Materialization & MzMap::get(Uuid id)
{
  lock_guard<mutex> lk{mtx_};
  return data_[id];
}

// InterfaceMzInfo -------------------------------------------------------------

Json InterfaceMzInfo::json() const
{
  Json j;
  j["ipv4"] = ipaddr_v4.json();
  return j;
}

InterfaceMzInfo InterfaceMzInfo::fromJson(Json j)
{
  InterfaceMzInfo x;
  x.ipaddr_v4 = IpV4Address::fromJson(extract(j, "ipv4", "InterfaceMzInfo"));
  return x;
}

// ComputerMzInfo --------------------------------------------------------------

Json ComputerMzInfo::json() const
{
  Json j;
  switch(launchState)
  {
    case LaunchState::None: j["launch-state"] = "none"; break;
    case LaunchState::Queued: j["launch-state"] = "queued"; break;
    case LaunchState::Launching: j["launch-state"] = "launching"; break;
    case LaunchState::Up: j["launch-state"] = "up"; break;
  }

  vector<Json> ifxs;
  for(const auto & p : interfaces)
  {
    Json x;
    x["id"] = p.first;
    x["info"] = p.second.json();
    ifxs.push_back(x);
  }
  j["interfaces"] = ifxs;
  return j;
}

ComputerMzInfo ComputerMzInfo::fromJson(Json j)
{
  ComputerMzInfo x;
  string launch_state = extract(j, "launch-state", "ComputerMzInfo");
  if(launch_state == "none") x.launchState = LaunchState::None;
  else if(launch_state == "queued") x.launchState = LaunchState::Queued;
  else if(launch_state == "launching") x.launchState = LaunchState::Launching;
  else if(launch_state == "up") x.launchState = LaunchState::Up;

  Json ifx = extract(j, "interfaces", "ComputerMzInfo");
  for(const Json & ix : ifx)
  {
    string id = extract(ix, "id", "InterfaceMzInfo");
    InterfaceMzInfo info = 
      InterfaceMzInfo::fromJson(extract(ix, "info", "InterfaceMzInfo"));
    x.interfaces[id] = info;
  }

  return x;
}
