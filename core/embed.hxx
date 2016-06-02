#ifndef DNA_ENV_EMBED
#define DNA_ENV_EMBED

#include <vector>
#include "topo.hxx"

namespace marina {

TestbedTopology embed(Blueprint, TestbedTopology);
TestbedTopology unembed(Blueprint, TestbedTopology);

}

#endif
