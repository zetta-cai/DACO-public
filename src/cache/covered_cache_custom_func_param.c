#include "cache/covered_cache_custom_func_param.h"

namespace covered
{
    // UpdateIsNeighborCachedFlagFuncParam

    const std::string UpdateIsNeighborCachedFlagFuncParam::kClassName("UpdateIsNeighborCachedFlagFuncParam");

    const std::string UpdateIsNeighborCachedFlagFuncParam::FUNCNAME("update_is_neighbor_cached_flag");

    UpdateIsNeighborCachedFlagFuncParam::UpdateIsNeighborCachedFlagFuncParam(const Key& key, const bool& is_neighbor_cached)
        : CacheCustomFuncParamBase(true, true, key, true), is_neighbor_cached_(is_neighbor_cached)
    {
    }

    UpdateIsNeighborCachedFlagFuncParam::~UpdateIsNeighborCachedFlagFuncParam()
    {
    }

    bool UpdateIsNeighborCachedFlagFuncParam::isNeighborCached() const
    {
        return is_neighbor_cached_;
    }

    // GetLocalSyncedVictimCacheinfosParam

    const std::string GetLocalSyncedVictimCacheinfosParam::kClassName("GetLocalSyncedVictimCacheinfosParam");

    const std::string GetLocalSyncedVictimCacheinfosParam::FUNCNAME("get_local_synced_victim_cacheinfos");

    GetLocalSyncedVictimCacheinfosParam::GetLocalSyncedVictimCacheinfosParam()
        : CacheCustomFuncParamBase(false, false, Key(), false)
    {
        // NOTE: as we only access local edge cache (thread safe w/o per-key rwlock) instead of validity map (thread safe w/ per-key rwlock), we do NOT need to acquire a fine-grained read lock here

        // NOTE: for is_perkey_write_lock_ and key_, will NOT be used by EdgeWrapper if NOT need per-key lock

        // NOTE: for is_local_cache_write_lock_, will acquire a read lock to check local metadata atomically

        victim_cacheinfos_.clear();
    }

    GetLocalSyncedVictimCacheinfosParam::~GetLocalSyncedVictimCacheinfosParam()
    {
    }

    std::list<VictimCacheinfo>& GetLocalSyncedVictimCacheinfosParam::getVictimCacheinfosRef()
    {
        return victim_cacheinfos_;
    }

    const std::list<VictimCacheinfo>& GetLocalSyncedVictimCacheinfosParam::getVictimCacheinfosConstRef() const
    {
        return victim_cacheinfos_;
    }

    // GetCollectedPopularityParam

    const std::string GetCollectedPopularityParam::kClassName("GetCollectedPopularityParam");

    const std::string GetCollectedPopularityParam::FUNCNAME("get_collected_popularity");

    GetCollectedPopularityParam::GetCollectedPopularityParam(const Key& key)
        : CacheCustomFuncParamBase(true, false, key, false)
    {
        // NOTE: for is_local_cache_write_lock_, will acquire a read lock to get local metadata atomically
    }

    GetCollectedPopularityParam::~GetCollectedPopularityParam()
    {
    }

    CollectedPopularity GetCollectedPopularityParam::getCollectedPopularity() const
    {
        return collected_popularity_;
    }

    void GetCollectedPopularityParam::setCollectedPopularity(const CollectedPopularity& other)
    {
        collected_popularity_ = other;
    }
}