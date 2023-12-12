/*
 * CoveredCooperationWrapper: basic cooperative caching framework for baselines.
 *
 * NOTE: all non-const shared variables in CoveredCooperationWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.10.12).
 */

#ifndef COVERED_COOPERATION_WRAPPER_H
#define COVERED_COOPERATION_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"

namespace covered
{
    class CoveredCooperationWrapper : public CooperationWrapperBase
    {
    public:
        CoveredCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~CoveredCooperationWrapper();

        // (0) Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper) (ONLY for COVERED)

        virtual std::list<std::pair<Key, DirinfoSet>> getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset) const override; // NOTE: all edge cache/beacon/invalidation servers will access cooperation wrapper to get content directory information for local beaconed victims from received victim syncset
        virtual std::list<std::pair<Key, DirinfoSet>> getLocalBeaconedVictimsFromCacheinfos(const std::list<VictimCacheinfo>& victim_cacheinfos) const override;
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif