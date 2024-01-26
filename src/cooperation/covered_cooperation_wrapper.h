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

#include "cooperation/cooperation_wrapper_base.h"
#include "cooperation/covered_cooperation_custom_func_param.h"

namespace covered
{
    class CoveredCooperationWrapper : public CooperationWrapperBase
    {
    public:
        CoveredCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~CoveredCooperationWrapper();

        // (0) Cache-method-specific custom functions

        virtual void constCustomFunc(const std::string& funcname, CooperationCustomFuncParamBase* func_param_ptr) const override;

        // COVERED-specific internal functions

        // Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper)
        void getLocalBeaconedVictimsFromVictimSyncsetInternal_(const VictimSyncset& victim_syncset, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_neighbor_synced_victim_dirinfosets) const; // NOTE: all edge cache/beacon/invalidation servers will access cooperation wrapper to get content directory information for local beaconed victims from received victim syncset
        void getLocalBeaconedVictimsFromCacheinfosInternal_(const std::list<VictimCacheinfo>& victim_cacheinfos, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victim_dirinfosets) const;
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif