#include "cooperation/covered_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredCooperationWrapper::kClassName("CoveredCooperationWrapper");

    CoveredCooperationWrapper::CoveredCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name) : CooperationWrapperBase(edgecnt, edge_idx, hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredCooperationWrapper::~CoveredCooperationWrapper() {}

    // (0) Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper) (ONLY for COVERED)

    void CoveredCooperationWrapper::getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_neighbor_synced_victim_dirinfosets) const
    {
        checkPointers_();

        // Get victim cacheinfos of neighbor synced victims (complete/compressed)
        std::list<VictimCacheinfo> neighbor_synced_victims;
        bool with_complete_victim_syncset = victim_syncset.getLocalSyncedVictims(neighbor_synced_victims);
        UNUSED(with_complete_victim_syncset); // Transmitted victim syncset received from neighbor edge node can be either complete or compressed

        // Get directory info sets for neighbor synced victimed beaconed by the current edge node
        getLocalBeaconedVictimsFromCacheinfos(neighbor_synced_victims, local_beaconed_neighbor_synced_victim_dirinfosets); // NOTE: dirinfo sets from local directory table MUST be complete

        return;
    }

    void CoveredCooperationWrapper::getLocalBeaconedVictimsFromCacheinfos(const std::list<VictimCacheinfo>& victim_cacheinfos, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victim_dirinfosets) const
    {
        // NOTE: victim_cacheinfos is from local edge cache or neighbor edge node, which can be either complete or compressed

        for (std::list<VictimCacheinfo>::const_iterator iter = victim_cacheinfos.begin(); iter != victim_cacheinfos.end(); iter++)
        {
            // NOTE: local/neighbor synced victim MUST NOT invalid or fully-deduped
            MYASSERT(!iter->isInvalid());
            MYASSERT(!iter->isFullyDeduped());

            if (iter->isStale()) // NOTE: NO need to get local dirinfo set for a stale victim, as victim tracker will remove the corresponding victim cacheinfo and may also remove the victim dirinfo if refcnt becomes zero
            {
                continue;
            }

            const Key& tmp_key = iter->getKey();
            uint32_t beacon_edgeidx = 0;
            bool with_valid_beacon_edgeidx = iter->getBeaconEdgeidx(beacon_edgeidx);
            if (!with_valid_beacon_edgeidx)
            {
                beacon_edgeidx = getBeaconEdgeIdx(tmp_key);
            }
            bool current_is_beacon = (beacon_edgeidx == edge_idx_);
            if (current_is_beacon) // Key is beaconed by current edge node
            {
                DirinfoSet tmp_dirinfo_set = getLocalDirectoryInfos(tmp_key);
                MYASSERT(tmp_dirinfo_set.isComplete()); // NOTE: dirinfo sets from local directory table MUST be complete
                local_beaconed_victim_dirinfosets.push_back(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        return;
    }
}