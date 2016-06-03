#ifndef MARINA_CORE_DB_HXX
#define MARINA_CORE_DB_HXX

#include <string>
#include <3p/json/src/json.hpp>
#include <postgresql/libpq-fe.h>
#include "core/topo.hxx"
#include "core/blueprint.hxx"
#include "core/blueprint.hxx"

namespace marina
{
  class DB
  {
    public:
      DB(std::string address);
      ~DB();

      //blueprint
      std::string saveBlueprint(std::string project, Json src);  
      Blueprint fetchBlueprint(std::string project, std::string bp_name);
      std::vector<Blueprint> fetchBlueprints(std::string project);
      void deleteBlueprint(std::string project, std::string bp_name);

      //materialization
      std::string saveMaterialization(std::string project, std::string bpid, 
                                      Json mzn);
      Blueprint fetchMaterialization(std::string project, std::string bpid);
      void deleteMaterialization(std::string project, std::string bpid);

      //hardware topology
      void setHwTopo(Json topo);
      TestbedTopology fetchHwTopo();
      void deleteHwTopo();

      //vxlan
      size_t newVxlanVni(std::string netid);
      void freeVxlanVni(std::string netid);


    private:
      std::string address_;
      PGconn *conn_{nullptr};
  };

  struct DeleteActiveBlueprintError {};
}


#endif
