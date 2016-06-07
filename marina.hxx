#ifndef MARINATB_MARINA_HXX
#define MARINATB_MARINA_HXX

/*
 * a top level include file for marinatb
 */

#include "core/blueprint.hxx"
#include "core/topo.hxx"
#include "core/embed.hxx"
#include "3p/pipes/pipes.hxx"

#define BLUEPRINT extern "C" Blueprint bp()
#define TOPOLOGY extern "C" TestbedTopology topo()

#endif
