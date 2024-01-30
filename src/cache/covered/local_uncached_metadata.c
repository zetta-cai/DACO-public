#include "cache/covered/local_uncached_metadata.h"

#include <assert.h>

#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"

namespace covered
{
    const std::string LocalUncachedMetadata::kClassName("LocalUncachedMetadata");

    LocalUncachedMetadata::LocalUncachedMetadata(const uint64_t& max_bytes_for_uncached_objects) : CacheMetadataBase(), max_bytes_for_uncached_objects_(max_bytes_for_uncached_objects)
    {
        assert(max_bytes_for_uncached_objects > 0);
    }

    LocalUncachedMetadata::~LocalUncachedMetadata() {}

    uint64_t LocalUncachedMetadata::getSizeForCapacity() const
    {
        const bool is_local_cached_metadata = false;
        return getSizeForCapacity_(is_local_cached_metadata);   
    }

    // For newly-admited/tracked keys

    bool LocalUncachedMetadata::afterAddForNewKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt)
    {
        // Detrack key for uncached capacity limitation if necessary
        Key detracked_key;
        while (true)
        {
            bool need_detrack = needDetrackForUncachedObjects_(detracked_key);
            if (need_detrack) // Cache size usage for local uncached objects exceeds the max bytes limitation
            {
                const uint32_t detracked_keylen = detracked_key.getKeyLength();
                const uint32_t detracked_object_size = getObjectSize(detracked_key);
                uint32_t detracked_value_size = detracked_object_size >= detracked_keylen ? detracked_object_size - detracked_keylen: 0;
                removeForExistingKey(detracked_key, Value(detracked_value_size)); // For getrsp with cache miss, put/delrsp with cache miss (NOTE: this will remove value from auxiliary data cache if any)
            }
            else // Local uncached objects is limited
            {
                break;
            }
        }

        return false; // Local uncached metadata always returns false
    }

    // For existing keys

    bool LocalUncachedMetadata::isGlobalCachedForExistingKey(const Key& key) const
    {
        const HomoKeyLevelMetadata homo_key_level_metadata = getkeyLevelMetadata(key);
        return homo_key_level_metadata.isGlobalCached();
    }

    void LocalUncachedMetadata::updateIsGlobalCachedForExistingKey(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const bool& is_getrsp, const bool& is_global_cached)
    {
        assert(is_getrsp == true);

        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);

        // Update object-level is_global_cached flag of value-unrelated metadata for local getrsp with cache miss
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_lookup_iter->second.getPerkeyMetadataListIter();
        assert(is_global_cached != perkey_metadata_list_iter->second.isGlobalCached()); // is_global_cached flag MUST be changed
        perkey_metadata_list_iter->second.updateIsGlobalCached(is_global_cached);

        // NOTE: is_global_cached does NOT affect local uncached popularity
        /*// Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, perkey_metadata_list_iter->second, getGroupLevelMetadata_(perkey_lookup_iter));*/

        // Update reward
        Reward new_reward = calculateReward_(edge_wrapper_ptr, perkey_metadata_list_iter);
        sorted_reward_multimap_t::iterator new_sorted_reward_iter = updateReward_(new_reward, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, new_sorted_reward_iter);

        return;
    }

    bool LocalUncachedMetadata::beforeUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const
    {
        return false;
    }

    bool LocalUncachedMetadata::afterUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const
    {
        return false;
    }

    // For popularity information

    void LocalUncachedMetadata::getPopularity(const Key& key, Popularity& local_popularity, Popularity& redirected_popularity) const
    {
        const HomoKeyLevelMetadata& key_level_metadata_ref = getkeyLevelMetadata(key);
        local_popularity = key_level_metadata_ref.getLocalPopularity();
        redirected_popularity = 0.0; // Local uncached metadata does NOT have redirected popularity
        return;
    }

    void LocalUncachedMetadata::calculateAndUpdatePopularity_(perkey_metadata_list_t::iterator& perkey_metadata_list_iter, const HomoKeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref)
    {
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize object_size = key_level_metadata_ref.getObjectSize();
        #else
        ObjectSize objsize_size = static_cast<ObjectSize>(group_level_metadata_ref.getAvgObjectSize());
        #endif

        // Calculate and update local uncached popularity
        Frequency local_frequency = key_level_metadata_ref.getLocalFrequency();
        Popularity local_uncached_popularity = calculatePopularity_(local_frequency, object_size);
        perkey_metadata_list_iter->second.updateLocalPopularity(local_uncached_popularity);

        return;
    }

    // For reward information
    Reward LocalUncachedMetadata::calculateReward_(const EdgeWrapperBase* edge_wrapper_ptr, perkey_metadata_list_t::iterator perkey_metadata_list_iter) const
    {
        // Get local uncached popularity
        const Popularity local_uncached_popularity = perkey_metadata_list_iter->second.getLocalPopularity();
        const bool is_global_cached = perkey_metadata_list_iter->second.isGlobalCached();

        // Calculte local reward (i.e., min admission benefit, as the local edge node does NOT know cache miss status of all other edge nodes and conservatively treat it as a local single placement)
        CalcLocalUncachedRewardFuncParam tmp_param(local_uncached_popularity, is_global_cached);
        edge_wrapper_ptr->constCustomFunc(CalcLocalUncachedRewardFuncParam::FUNCNAME, &tmp_param);
        Reward local_reward = tmp_param.getRewardConstRef();

        return local_reward;
    }

    // ONLY for local uncached metadata

    bool LocalUncachedMetadata::needDetrackForUncachedObjects_(Key& detracked_key) const
    {
        //uint32_t cur_trackcnt = perkey_lookup_table_.size();
        uint64_t cache_size_usage_for_uncached_objects = getSizeForCapacity();
        if (cache_size_usage_for_uncached_objects > max_bytes_for_uncached_objects_)
        {
            Key tmp_key;
            Reward tmp_reward = 0.0;
            bool is_least_reward_key_exist = getLeastRewardKeyAndReward(0, tmp_key, tmp_reward);
            assert(is_least_reward_key_exist == true); // NOTE: the least reward uncached object MUST exist

            detracked_key = tmp_key;

            return true;
        }
        else
        {
            return false;
        }
    }
}