/*
 * HeteroKeyLevelMetadata: heterogeneous key-level metadata for each local cached object in local edge cache.
 *
 * NOTE: we will provide redirected cached popularity as a part of victim cacheinfo for victim synchronization of local synced victims by getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() and extra victim fetching for placement calculation by getLocalCacheVictimKeysInternal_().
 * 
 * NOTE: redirected cached popularity will be used to calculate eviction cost in the beacon edge node during cache placement by VictimTracker::calcEvictionCost().
 * 
 * By Siyuan Sheng (2023.11.16).
 */

#ifndef HETERO_KEY_LEVEL_METADATA_H
#define HETERO_KEY_LEVEL_METADATA_H

#include <string>

#include "cache/covered/key_level_metadata_base.h"
#include "common/covered_common_header.h"

namespace covered
{
    class HeteroKeyLevelMetadata : public KeyLevelMetadataBase
    {
    public:
        HeteroKeyLevelMetadata(const GroupId& group_id);
        HeteroKeyLevelMetadata(const HeteroKeyLevelMetadata& other);
        ~HeteroKeyLevelMetadata();

        virtual void updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached) override; // For getreq with redirected hits, update object-level value-unrelated metadata (is_global_cached MUST be true and will NOT be used)
        
        void updateRedirectedPopularity(const Popularity& redirected_popularity);

        Frequency getRedirectedFrequency() const;
        Popularity getRedirectedPopularity() const; // Get redirected popularity for redirected hits of local cached objectds

        static uint64_t getSizeForCapacity();
    private:
        static const std::string kClassName;

        // For redirected hits of local cached objects
        Frequency redirected_frequency_;
        Popularity redirected_popularity_;
    };
}

#endif