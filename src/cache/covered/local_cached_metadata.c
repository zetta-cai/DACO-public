#include "cache/covered/local_cached_metadata.h"

#include "common/util.h"
#include "common/covered_common_header.h"
#include "edge/covered_edge_custom_func_param.h"

namespace covered
{
    const std::string LocalCachedMetadata::kClassName("LocalCachedMetadata");

    LocalCachedMetadata::LocalCachedMetadata() : CacheMetadataBase() {}

    LocalCachedMetadata::~LocalCachedMetadata() {}

    uint64_t LocalCachedMetadata::getSizeForCapacity() const
    {
        const bool is_local_cached_metadata = true;
        return getSizeForCapacity_(is_local_cached_metadata);
    }

    // For beacon-based metadata update

    bool LocalCachedMetadata::checkIsNeighborCachedForExistingKey(const Key& key)
    {
        bool is_neighbor_cached = false;

        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);
        HeteroKeyLevelMetadata& key_level_metadata_ref = perkey_lookup_iter->second.getPerkeyMetadataListIter()->second;
        is_neighbor_cached = key_level_metadata_ref.isNeighborCached();

        return is_neighbor_cached;
    }

    void LocalCachedMetadata::updateIsNeighborCachedForExistingKey(const Key& key, const bool& is_neighbor_cached)
    {
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);
        HeteroKeyLevelMetadata& key_level_metadata_ref = perkey_lookup_iter->second.getPerkeyMetadataListIter()->second;
        if (is_neighbor_cached)
        {
            key_level_metadata_ref.enableIsNeighborCached();
        }
        else
        {
            key_level_metadata_ref.disableIsNeighborCached();
        }
        return;
    }

    // For newly-admited/tracked keys

    bool LocalCachedMetadata::afterAddForNewKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt)
    {
        return isAffectVictimTracker_(perkey_lookup_const_iter, peredge_synced_victimcnt);
    }

    // For existing keys

    bool LocalCachedMetadata::beforeUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const
    {
        return isAffectVictimTracker_(perkey_lookup_const_iter, peredge_synced_victimcnt);
    }

    bool LocalCachedMetadata::afterUpdateStatsForExistingKey_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const
    {
        return isAffectVictimTracker_(perkey_lookup_const_iter, peredge_synced_victimcnt);
    }

    // For popularity information

    void LocalCachedMetadata::getPopularity(const Key& key, Popularity& local_popularity, Popularity& redirected_popularity) const
    {
        const HeteroKeyLevelMetadata& key_level_metadata_ref = getkeyLevelMetadata(key);
        local_popularity = key_level_metadata_ref.getLocalPopularity();
        redirected_popularity = key_level_metadata_ref.getRedirectedPopularity();

        return;
    }
    void LocalCachedMetadata::getPopularity(const Key& key, Popularity& local_popularity, Popularity& redirected_popularity, Popularity& redirected_popularity_reward) const
    {
        const HeteroKeyLevelMetadata& key_level_metadata_ref = getkeyLevelMetadata(key);
        local_popularity = key_level_metadata_ref.getLocalPopularity();
        redirected_popularity = key_level_metadata_ref.getRedirectedPopularity();
        redirected_popularity_reward = key_level_metadata_ref.getRedirectedPopularityReward();

        return;
    }
    void LocalCachedMetadata::calculateAndUpdatePopularity_(perkey_metadata_list_t::iterator& perkey_metadata_list_iter, const HeteroKeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref)
    {
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize object_size = key_level_metadata_ref.getObjectSize();
        #else
        ObjectSize objsize_size = static_cast<ObjectSize>(group_level_metadata_ref.getAvgObjectSize());
        #endif

        // Calculate and update local cached popularity
        Frequency local_frequency = key_level_metadata_ref.getLocalFrequency();
        Popularity local_cached_popularity = calculatePopularity(local_frequency, object_size);
        perkey_metadata_list_iter->second.updateLocalPopularity(local_cached_popularity);

        // Calculate and update redirected cached popularity
        // NOTE: Redirected cached popularity is calculated based on static metadata (e.g., object size) from local requests, object-level metadata (e.g., frequency) for redirected requests, and group-level metadata for all requests
        Frequency redirected_frequency;
        Popularity redirected_cached_popularity; 
        Frequency redirected_frequency_reward;
        Popularity redirected_cached_popularity_reward; 
        redirected_frequency = key_level_metadata_ref.getRedirectedFrequency();
        redirected_cached_popularity = calculatePopularity(redirected_frequency, object_size);
        perkey_metadata_list_iter->second.updateRedirectedPopularity(redirected_cached_popularity);
        if(is_p2p_global_mode_in_common){
            // p2p mode: use p2p_redirected_reward_freq_ to calculate redirected popularity
            redirected_frequency_reward = key_level_metadata_ref.getRedirectedRewardFreq();
            redirected_cached_popularity_reward  = calculatePopularity(redirected_frequency_reward, object_size);
            perkey_metadata_list_iter->second.updateRedirectedPopularityReward(redirected_cached_popularity_reward);
            // if(redirected_frequency>0){
            //     // print redirected_frequency_reward for debug
            //     std::cout<<"[Debug] redirected_frequency_reward: "<<redirected_frequency_reward<<", redirected_frequency: "<<redirected_frequency<<std::endl;
            // }
        }


        return;
    }

    // For reward information
    // TODO for reward
    Reward LocalCachedMetadata::calculateReward_(const EdgeWrapperBase* edge_wrapper_ptr, perkey_metadata_list_t::iterator perkey_metadata_list_iter) const
    {
        // Get local/redirected cached popularity
        const Popularity local_cached_popularity = perkey_metadata_list_iter->second.getLocalPopularity();
        const Popularity redirected_cached_popularity = perkey_metadata_list_iter->second.getRedirectedPopularity();
        const bool is_neighbor_cached = perkey_metadata_list_iter->second.isNeighborCached();

        // Calculate local reward (i.e., max eviction cost, as the local edge node does NOT know cache hit status of all other edge nodes and conservatively treat it as the last copy)
        const bool is_last_copies = !is_neighbor_cached;
        CalcLocalCachedRewardFuncParam tmp_param(local_cached_popularity, redirected_cached_popularity, is_last_copies);
        edge_wrapper_ptr->constCustomFunc(CalcLocalCachedRewardFuncParam::FUNCNAME, &tmp_param);
        Reward local_reward = tmp_param.getRewardConstRef();

        return local_reward;
    }

    // ONLY for local cached metadata

    bool LocalCachedMetadata::isAffectVictimTracker_(const typename perkey_lookup_table_t::const_iterator& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const
    {
        bool affect_victim_tracker = false;

        // NOTE: OBSOLETE due to time-consuming
        // uint32_t least_reward_rank = getLeastRewardRank_(perkey_lookup_const_iter);
        // if (least_reward_rank < peredge_synced_victimcnt) // If key was a local synced victim before
        // {
        //     affect_victim_tracker = true;
        // }

        affect_victim_tracker = isWithinTargetLeastRewardRank_(perkey_lookup_const_iter, peredge_synced_victimcnt);

        return affect_victim_tracker;
    }
}