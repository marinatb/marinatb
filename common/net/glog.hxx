#ifndef MARINA_COMMON_NET_GLOG
#define MARINA_COMMON_NET_GLOG

#include <string>
#include <glog/logging.h>

namespace marina
{
  struct Glog
  {
    static bool initialized;
    static void init(std::string service_name);
  };
}

#endif
