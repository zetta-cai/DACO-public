/*
 * Different custom function parameters for COVERED.
 *
 * By Siyuan Sheng (2024.01.10).
 */

#ifndef COVERED_CUSTOM_FUNC_PARAM_H
#define COVERED_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "cache/custom_func_param_base.h"
#include "common/key.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    // UpdateIsNeighborCachedFlagFuncParam

    class UpdateIsNeighborCachedFlagFuncParam : public CustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Update is_neighbor_cached flag in local cached metadata (for beacon-based local cache metadata udpate)

        UpdateIsNeighborCachedFlagFuncParam(const Key& key, const bool& is_neighbor_cached);
        virtual ~UpdateIsNeighborCachedFlagFuncParam();

        bool isNeighborCached() const;
    private:
        static const std::string kClassName;

        const bool is_neighbor_cached_;
    };

    // GetLocalSyncedVictimCacheinfosParam

    class GetLocalSyncedVictimCacheinfosParam : public CustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Get up to peredge_synced_victimcnt local synced victims with the least local rewards (for victim synchronization)

        GetLocalSyncedVictimCacheinfosParam();
        virtual ~GetLocalSyncedVictimCacheinfosParam();

        std::list<VictimCacheinfo>& getVictimCacheinfosRef();
        const std::list<VictimCacheinfo>& getVictimCacheinfosConstRef() const;
    private:
        static const std::string kClassName;

        std::list<VictimCacheinfo> victim_cacheinfos_;
    };

    // GetCollectedPopularityParam

    class GetCollectedPopularityParam : public CustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Get collected popularity of local uncached objects (for piggybacking-based popularity colleciton)

        GetCollectedPopularityParam(const Key& key);
        virtual ~GetCollectedPopularityParam();

        CollectedPopularity getCollectedPopularity() const;
        void setCollectedPopularity(const CollectedPopularity& other);
    private:
        static const std::string kClassName;

        CollectedPopularity collected_popularity_;
    };
}

#endif