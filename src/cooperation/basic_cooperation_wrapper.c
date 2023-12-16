#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name) : CooperationWrapperBase(edgecnt, edge_idx, hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}

    // (0) Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper) (ONLY for COVERED)

    void BasicCooperationWrapper::getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_neighbor_synced_victim_dirinfosets) const
    {
        Util::dumpErrorMsg(instance_name_, "Baselines should NOT invoke getLocalBeaconedVictimsFromVictimSyncset(), which is ONLY for COVERED!");
        exit(1);
    }

    void BasicCooperationWrapper::getLocalBeaconedVictimsFromCacheinfos(const std::list<VictimCacheinfo>& victim_cacheinfos, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victim_dirinfosets) const
    {
        Util::dumpErrorMsg(instance_name_, "Baselines should NOT invoke getLocalBeaconedVictimsFromCacheinfos(), which is ONLY for COVERED!");
        exit(1);
    }
}