/*
 * LocalUncachedMetadata: metadata for local cached objects in covered.
 *
 * NOTE: for each local uncached object, local reward (i.e., min admission benefit, as the local edge node does NOT know cache miss status of all other edge nodes and conservatively treat it as a local single placement) is calculated by local uncached popularity with heterogeneous weights.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_UNCACHED_METADATA_H
#define LOCAL_UNCACHED_METADATA_H

#include <string>

#include "cache/covered/homo_key_level_metadata.h"
#include "cache/covered/cache_metadata_base_impl.h"

namespace covered
{
    class LocalUncachedMetadata : public CacheMetadataBase<HomoKeyLevelMetadata>
    {
    public:
        LocalUncachedMetadata(const uint64_t& max_bytes_for_uncached_objects);
        virtual ~LocalUncachedMetadata();

        // For existig key
        bool isGlobalCachedForExistingKey(const Key& key) const;
        void updateIsGlobalCachedForExistingKey(const EdgeWrapper* edge_wrapper_ptr, const Key& key, const bool& is_getrsp, const bool& is_global_cached); // NOTE: ONLY for getrsp (put/delreq will update is_global_cached when update value-unrelated metadata)

        // For popularity information
        virtual void getPopularity(const Key& key, Popularity& local_popularity, Popularity& redirected_popularity) const override; // Local uncached metadata NOT set redirected_popularity

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local uncached objects
    private:
        static const std::string kClassName;

        // For newly-admited/tracked keys
        virtual bool afterAddForNewKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) override; // Always return false

        // For existing key
        virtual bool beforeUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const override; // Always return false
        virtual bool afterUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const override; // Always return false

        // For popularity information
        virtual void calculateAndUpdatePopularity_(perkey_metadata_list_t::iterator& perkey_metadata_iter, const HomoKeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref) override; // Calculate local uncached popularity based on object-level metadata for local misses and group-level metadata for all requests

        // For reward information
        virtual Reward calculateReward_(const EdgeWrapper* edge_wrapper_ptr, perkey_metadata_list_t::iterator perkey_metadata_iter) const override;

        // ONLY for local uncached metadata
        bool needDetrackForUncachedObjects_(Key& detracked_key) const; // Check if need to detrack the least popular key for local uncached object

        const uint64_t max_bytes_for_uncached_objects_; // Used only for local uncached objects
    };
}

#endif