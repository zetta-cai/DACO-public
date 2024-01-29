/*
 * LocalCachedMetadata: metadata for local cached objects in covered.
 *
 * NOTE: for each local cached object, local reward (i.e., max eviction cost, as the local edge node does NOT know cache hit status of all other edge nodes and conservatively treat it as the last copy) is calculated by heterogeneous local/redirected cached popularity.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_CACHED_METADATA_H
#define LOCAL_CACHED_METADATA_H

#include <string>

#include "cache/covered/hetero_key_level_metadata.h"
#include "cache/covered/cache_metadata_base_impl.h"

namespace covered
{
    class LocalCachedMetadata : public CacheMetadataBase<HeteroKeyLevelMetadata>
    {
    public:
        LocalCachedMetadata();
        virtual ~LocalCachedMetadata();

        // For beacon-based metadata update
        bool checkIsNeighborCachedForExistingKey(const Key& key);
        void updateIsNeighborCachedForExistingKey(const Key& key, const bool& is_neighbor_cached);

        // For popularity information
        virtual void getPopularity(const Key& key, Popularity& local_popularity, Popularity& redirected_popularity) const override;

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local cached objects
    private:
        static const std::string kClassName;

        // For newly-admited/tracked keys
        virtual bool afterAddForNewKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) override; // Return if affect victim tracker

        // For existing key
        virtual bool beforeUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const override; // Return if affect local synced victims in victim tracker
        virtual bool afterUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const override; // Return if affect local synced victims in victim tracker

        // For popularity information
        virtual void calculateAndUpdatePopularity_(perkey_metadata_list_t::iterator& perkey_metadata_iter, const HeteroKeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref) override; // Calculate local/redirected uncached popularity based on object-level metadata for local/redirected hits and group-level metadata for all requests

        // For reward information
        virtual Reward calculateReward_(const EdgeWrapperBase* edge_wrapper_ptr, perkey_metadata_list_t::iterator perkey_metadata_iter) const override;

        // ONLY for local cached metadata
        bool isAffectVictimTracker_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const;
    };
}

#endif