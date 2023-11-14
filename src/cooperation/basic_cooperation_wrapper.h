/*
 * BasicCooperationWrapper: basic cooperative caching framework for baselines.
 *
 * NOTE: all non-const shared variables in BasicCooperationWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef BASIC_COOPERATION_WRAPPER_H
#define BASIC_COOPERATION_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"

namespace covered
{
    class BasicCooperationWrapper : public CooperationWrapperBase
    {
    public:
        BasicCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~BasicCooperationWrapper();

        // (0) Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper) (ONLY for COVERED)

        virtual std::unordered_map<Key, DirinfoSet, KeyHasher> getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset) const override; // NOTE: all edge cache/beacon/invalidation servers will access cooperation wrapper to get content directory information for local beaconed victims from received victim syncset
        virtual std::unordered_map<Key, DirinfoSet, KeyHasher> getLocalBeaconedVictimsFromCacheinfos(const std::list<VictimCacheinfo>& victim_cacheinfos) const override;
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif