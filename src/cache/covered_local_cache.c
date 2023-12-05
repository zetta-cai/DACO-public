#include "cache/covered_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string CoveredLocalCache::UPDATE_IS_NEIGHBOR_CACHED_FLAG_FUNC_NAME("update_is_neighbor_cached_flag");

    const std::string CoveredLocalCache::kClassName("CoveredLocalCache");

    CoveredLocalCache::CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt) : LocalCacheBase(edge_idx, capacity_bytes), peredge_synced_victimcnt_(peredge_synced_victimcnt), local_cached_metadata_(), local_uncached_metadata_(local_uncached_capacity_bytes)
    {
        // (A) Const variable

        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // (B) Non-const shared variables of local cached objects for eviction

        // TommyDS-based key-value storage
        // NOTE: NO need to introduce COMMON_ENGINE_INTERNAL_UNUSED_CAPACITY_BYTES due to NO internal eviction of TommyDS engine
        internal_kvbytes_ = 0;
        covered_cache_ptr_ = new tommy_hashdyn;
        assert(covered_cache_ptr_ != NULL);
        tommy_hashdyn_init(covered_cache_ptr_);
    }

    CoveredLocalCache::~CoveredLocalCache()
    {
        // TMPDEBUG231201
        uint64_t internal_size = internal_kvbytes_;
        uint64_t local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
        uint64_t local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
        Util::dumpVariablesForDebug(instance_name_, 6, "internal_size:", std::to_string(internal_size).c_str(), "local_cached_metadata_size:", std::to_string(local_cached_metadata_size).c_str(), "local_uncached_metadata_size:", std::to_string(local_uncached_metadata_size).c_str());

        // TMPDEBUG231201
        uint64_t tommyds_internal_size = tommy_hashdyn_memory_usage(covered_cache_ptr_);
        uint32_t tommyds_objcnt = tommy_hashdyn_count(covered_cache_ptr_);
        uint64_t tommyds_kv_total_size = 0;
        void (*tmp_func)(void*, void*) = [](void* arg, void* obj)
        {
            uint64_t* total_size_ptr = (uint64_t*)(arg);
            tommyds_object_t* obj_ptr = (tommyds_object_t*)(obj);
            *total_size_ptr += (obj_ptr->key.getKeyLength() + obj_ptr->val.getValuesize());
        };
        tommy_hashdyn_foreach_arg(covered_cache_ptr_, tmp_func, &tommyds_kv_total_size);
        Util::dumpVariablesForDebug(instance_name_, 6, "tommyds_internal_size:", std::to_string(tommyds_internal_size).c_str(), "tommyds_objcnt:", std::to_string(tommyds_objcnt).c_str(), "tommyds_kv_total_size:", std::to_string(tommyds_kv_total_size).c_str());

        // Free TommyDS objects allocated outside TommyDS
        std::vector<tommyds_object_t*> obj_ptr_vec;
        void (*obj_free_func_ptr)(void*, void*) = [](void* arg, void* obj)
        {
            assert(arg != NULL);
            assert(obj != NULL);

            std::vector<tommyds_object_t*>* obj_ptr_vec_ptr = (std::vector<tommyds_object_t*>*)(arg);
            tommyds_object_t* obj_ptr = (tommyds_object_t*)(obj);
            obj_ptr_vec_ptr->push_back(obj_ptr);
            return;
        };
        tommy_hashdyn_foreach_arg(covered_cache_ptr_, obj_free_func_ptr, &obj_ptr_vec);
        assert(obj_ptr_vec.size() == tommy_hashdyn_count(covered_cache_ptr_));
        for (uint32_t i = 0; i < obj_ptr_vec.size(); i++)
        {
            tommyds_object_t* tmp_obj_ptr = obj_ptr_vec[i];
            assert(tmp_obj_ptr != NULL);
            delete tmp_obj_ptr;
            tmp_obj_ptr = NULL;
        }

        // Free hashtable buckets allocated inside TommyDS
        tommy_hashdyn_done(covered_cache_ptr_);

        // Free TommyDS itself
        delete covered_cache_ptr_;
        covered_cache_ptr_ = NULL;
    }

    const bool CoveredLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool CoveredLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        tommyds_object_t* tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));
        bool is_cached = (tmp_obj_ptr != NULL);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool CoveredLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        affect_victim_tracker = false;

        tommyds_object_t* tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));

        bool is_local_cached = (tmp_obj_ptr != NULL);
        if (is_local_cached)
        {
            value = tmp_obj_ptr->val;

            // Update local cached metadata for getreq with valid/invalid cache hit (ONLY value-unrelated metadata)
            const bool is_global_cached = true; // Local cached objects MUST be global cached
            affect_victim_tracker = local_cached_metadata_.updateNoValueStatsForExistingKey(key, peredge_synced_victimcnt_, is_redirected, is_global_cached);
        }
        else // key is NOT cached
        {
            // NOTE: we ONLY update local uncached metadata for local getreq w/ cache miss instead of redirected getreq w/ cache miss
            if (!is_redirected)
            {
                bool is_tracked = local_uncached_metadata_.isKeyExist(key);
                if (is_tracked) // Key is already tracked
                {
                    // Update local uncached metadata for getreq with cache miss (ONLY value-unrelated metadata)
                    const bool original_is_global_cached = local_uncached_metadata_.isGlobalCachedForExistingKey(key); // Conservatively keep original is_global_cached flag
                    local_uncached_metadata_.updateNoValueStatsForExistingKey(key, peredge_synced_victimcnt_, is_redirected, original_is_global_cached);

                    // NOTE: NOT update value-related metadata, as we conservatively treat the objsize unchanged (okay due to read-intensive edge cache trace)
                }
                else // Key will be newly tracked
                {
                    // NOTE: NOT track local uncached object into local uncached metadata by addForNewKey() here, as the get request of untracked object with (the first) cache miss cannot provide object size information to initialize and update local uncached metadata, and also cannot trigger placement
                }
            }
        }

        // NOTE: for getreq with cache miss, we will update local uncached metadata for getres by updateLocalUncachedMetadataForRspInternal_(key)

        return is_local_cached;
    }

    std::list<VictimCacheinfo> CoveredLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        for (uint32_t least_popular_rank = 0; least_popular_rank < peredge_synced_victimcnt_; least_popular_rank++)
        {
            Key tmp_victim_key;
            ObjectSize tmp_victim_object_size = 0;
            Popularity tmp_local_cached_popularity = 0.0;
            Popularity tmp_redirected_cached_popularity = 0.0;
            Reward tmp_local_reward = 0.0;

            bool is_least_popular_key_exist = local_cached_metadata_.getLeastRewardKeyAndReward(least_popular_rank, tmp_victim_key, tmp_local_reward);

            if (is_least_popular_key_exist)
            {
                tmp_victim_object_size = local_cached_metadata_.getObjectSize(tmp_victim_key);
                local_cached_metadata_.getPopularity(tmp_victim_key, tmp_local_cached_popularity, tmp_redirected_cached_popularity);

                uint32_t tmp_victim_value_size = 0;
                tommyds_object_t* tmp_victim_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &tmp_victim_key, hashForTommyds_(tmp_victim_key));
                assert(tmp_victim_ptr != NULL); // Victim must be cached before eviction
                tmp_victim_value_size = tmp_victim_ptr->val.getValuesize();
                #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
                // NOTE: tmp_victim_object_size got from key-level metadata is already accurate
                assert(tmp_victim_object_size == (tmp_victim_key.getKeyLength() + tmp_victim_value_size));
                #else
                // NOTE: tmp_victim_object_size got from group-level metadata is approximate, which should be replaced with the accurate one from local edge cache
                tmp_victim_object_size = tmp_victim_key.getKeyLength() + tmp_victim_value_size;
                #endif

                VictimCacheinfo tmp_victim_info(tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);
                assert(tmp_victim_info.isComplete()); // NOTE: victim cacheinfos from local edge cache MUST be complete
                local_synced_victim_cacheinfos.push_back(tmp_victim_info); // Add to the tail of the list
            }
            else
            {
                break;
            }
        }

        assert(local_synced_victim_cacheinfos.size() <= peredge_synced_victimcnt_);

        return local_synced_victim_cacheinfos;
    }

    void CoveredLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        ObjectSize object_size = 0;
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = local_uncached_metadata_.isKeyExist(key);
        if (is_key_tracked)
        {
            object_size = local_uncached_metadata_.getObjectSize(key);

            Popularity unused_redirection_popularity = 0.0;
            local_uncached_metadata_.getPopularity(key, local_uncached_popularity, unused_redirection_popularity);
            UNUSED(unused_redirection_popularity);
        }

        collected_popularity = CollectedPopularity(is_key_tracked, local_uncached_popularity, object_size);

        return;
    }

    bool CoveredLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        affect_victim_tracker = false;
        is_successful = false;
        const bool is_redirected = false; // NOTE: getrsp/putreq/delreq MUST NOT redirected getreq

        // Check is local cached
        tommyds_object_t* tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));
        bool is_local_cached = (tmp_obj_ptr != NULL);

        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            Value original_value = tmp_obj_ptr->val; // Keep original value
            tmp_obj_ptr->val = value; // Update with the latest valid value

            // Update internal key-value pair size usage
            const uint32_t original_valuesize = original_value.getValuesize();
            const uint32_t current_valuesize = value.getValuesize();
            if (original_valuesize < current_valuesize) // Value size increases
            {
                internal_kvbytes_ = Util::uint64Add(internal_kvbytes_, (current_valuesize - original_valuesize));
            }
            else if (original_valuesize > current_valuesize) // Value size decreases
            {
                internal_kvbytes_ = Util::uint64Minus(internal_kvbytes_, (original_valuesize - current_valuesize));
            }
            // Do nothing if value does NOT change

            // Update local cached metadata for getrsp with invalid hit and put/delreq with cache hit (both value-unrelated and value-related)
            if (is_getrsp) // getrsp with invalid cache hit
            {
                // NOTE: ONLY update value-related metadata, as value-unrelated metadata has already been updated for the corresponding getreq before in getLocalCacheInternal_()
                affect_victim_tracker = local_cached_metadata_.updateValueStatsForExistingKey(key, value, original_value, peredge_synced_victimcnt_);
            }
            else
            {
                const bool tmp_is_global_cached = true; // Local cached objects MUST be global cached
                bool tmp_affect_victim_tracker0 = local_cached_metadata_.updateNoValueStatsForExistingKey(key, peredge_synced_victimcnt_, is_redirected, tmp_is_global_cached);
                bool tmp_affect_victim_tracker1 = local_cached_metadata_.updateValueStatsForExistingKey(key, value, original_value, peredge_synced_victimcnt_);
                affect_victim_tracker = tmp_affect_victim_tracker0 || tmp_affect_victim_tracker1;
            }

            is_successful = true;
        }
        else // Key is NOT local cached
        {
            bool is_key_tracked = local_uncached_metadata_.isKeyExist(key);

            const uint32_t keylen = key.getKeyLength();
            if (!is_valid_objsize) // Large object size -> NOT track the key in local uncached metadata
            {
                if (is_key_tracked) // Key is already tracked by local uncached metadata
                {
                    const uint32_t original_object_size = local_uncached_metadata_.getObjectSize(key);
                    const uint32_t original_value_size = original_object_size >= keylen ? original_object_size - keylen : 0;
                    local_uncached_metadata_.removeForExistingKey(key, Value(original_value_size)); // Remove original value size to detrack the key from local uncached metadata
                }
            }
            else // With valid object size
            {
                if (!is_key_tracked) // Key will be newly tracked by local uncached metadata
                {
                    // Initialize and update local uncached metadata for getrsp/put/delreq with cache miss (both value-unrelated and value-related)
                    // NOTE: if the latest local uncached popularity of the newly-tracked key is NOT detracked in addForNewKey() under local uncached metadata capacity limitation, local/remote release writelock (for put/delreq) or local/remote directory lookup for the next getreq (for getrsp) will get the latest local uncached popularity for popularity aggregation to trigger placement calculation
                    const bool is_neighbor_cached = false; // NOTE: NEVER used by local uncached metadata
                    local_uncached_metadata_.addForNewKey(key, value, peredge_synced_victimcnt_, is_global_cached, is_neighbor_cached); // NOTE: peredge_synced_victimcnt_ will NOT be used for local uncached metadata
                }
                else // Key is tracked by local uncached metadata
                {
                    const uint32_t original_object_size = local_uncached_metadata_.getObjectSize(key);
                    const uint32_t original_value_size = original_object_size >= keylen ? original_object_size - keylen : 0;
                    
                    // Update local uncached value-related metadata for put/delreq with cache miss (NOT for getrsp with cache miss)
                    if (is_getrsp) // getrsp with cache miss
                    {
                        // NOTE: NOT update local uncached value-unrelated metadata for getrsp with cache miss, which has been done in getLocalCacheInternal_() for the corresponding getreq before
                        // NOTE: also NOT update local uncached value-related metadata for getrsp with cache miss, as getreq will NOT update the value of the uncached object

                        const bool original_is_global_cached = local_uncached_metadata_.isGlobalCachedForExistingKey(key);
                        if (is_global_cached != original_is_global_cached) // If is_global_cached flag changes
                        {
                            local_uncached_metadata_.updateIsGlobalCachedForExistingKey(key, is_getrsp, is_global_cached); // Update is_global_cached and reward
                        }
                    }
                    else // put/delreq with cache miss
                    {
                        // Update local uncached value-unrelated metadata for put/delreq with cache miss
                        local_uncached_metadata_.updateNoValueStatsForExistingKey(key, peredge_synced_victimcnt_, is_redirected, is_global_cached);
                        
                        local_uncached_metadata_.updateValueStatsForExistingKey(key, value, original_value_size, peredge_synced_victimcnt_); // NOTE: peredge_synced_victimcnt_ will NOT be used by local uncached metadata
                    }
                }
            }
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool CoveredLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(key);
        UNUSED(value);
        
        // COVERED will NEVER invoke this function for independent admission
        return false;
    }

    void CoveredLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        affect_victim_tracker = false;
        is_successful = false;

        tommyds_object_t* tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));
        bool is_local_cached = (tmp_obj_ptr != NULL);
    
        // assert(!is_local_cached); // (OBSOLETE) Key should NOT exist

        // NOTE: cache server worker and beacon server may perform cache placement for the same key simultaneously (triggered by local/remote directory lookup) and CoveredManager::placementCalculation_() is NOT atomic -> current edge node may receive duplicate local/remote placement notification
        if (is_local_cached)
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " is already local cached before admitLocalCacheInternal_(), which may be caused by occasional duplicate placement notification due to NOT strong atomicity of CoveredCacheManager";
            Util::dumpInfoMsg(instance_name_, oss.str());

            is_successful = true;
            return;
        }

        // TMPDEBUG231201
        uint64_t internal_size = internal_kvbytes_;
        uint64_t local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
        uint64_t local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
        Util::dumpVariablesForDebug(instance_name_, 6, "[before admit] internal_size:", std::to_string(internal_size).c_str(), "local_cached_metadata_size:", std::to_string(local_cached_metadata_size).c_str(), "local_uncached_metadata_size:", std::to_string(local_uncached_metadata_size).c_str());

        // Admit new key-value pair into internal TommyDS
        tommyds_object_t* tmp_allocate_obj_ptr = new tommyds_object_t();
        assert(tmp_allocate_obj_ptr != NULL);
        tmp_allocate_obj_ptr->key = key;
        tmp_allocate_obj_ptr->val = value;
        tommy_hashdyn_insert(covered_cache_ptr_, &tmp_allocate_obj_ptr->node, tmp_allocate_obj_ptr, hashForTommyds_(key));

        // Update internal key-value pair size usage
        internal_kvbytes_ = Util::uint64Add(internal_kvbytes_, (key.getKeyLength() + value.getValuesize()));

        // Update local cached metadata for admission
        const bool is_global_cached = true; // Local cached objects MUST be global cached
        affect_victim_tracker = local_cached_metadata_.addForNewKey(key, value, peredge_synced_victimcnt_, is_global_cached, is_neighbor_cached);

        // TMPDEBUG231201
        uint64_t after_internal_size = internal_kvbytes_;
        uint64_t after_local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
        uint64_t after_local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
        Util::dumpVariablesForDebug(instance_name_, 6, "[after admit] internal_size:", std::to_string(after_internal_size).c_str(), "local_cached_metadata_size:", std::to_string(after_local_cached_metadata_size).c_str(), "local_uncached_metadata_size:", std::to_string(after_local_uncached_metadata_size).c_str());

        // TMPDEBUG231201
        Util::dumpVariablesForDebug(instance_name_, 8, "newly-admited key", key.getKeystr().c_str(), "passed is_neighbor_cached:", Util::toString(is_neighbor_cached).c_str(), "is_neighbor_cached:", Util::toString(local_cached_metadata_.checkIsNeighborCachedForExistingKey(key)).c_str(), "local reward:", std::to_string(local_cached_metadata_.getLocalRewardForExistingKey(key)).c_str());

        // Remove from local uncached metadata if necessary for admission
        if (local_uncached_metadata_.isKeyExist(key))
        {
            // NOTE: get/put/delrsp with cache miss MUST already udpate local uncached metadata with the current value before admission, so we can directly use current value to remove it from local uncached metadata instead of approximate value
            local_uncached_metadata_.removeForExistingKey(key, value);
        }

        is_successful = true;

        return;
    }

    bool CoveredLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        // TODO: this function will be invoked at each candidate neighbor node by the beacon node for lazy fetching of candidate victims
        // TODO: this function will also be invoked at each placement neighbor node by the beacon node for cache placement
        
        assert(hasFineGrainedManagement());

        bool has_victim_key = false;
        uint32_t least_popular_rank = 0;
        uint64_t conservative_victim_total_size = 0;
        while (conservative_victim_total_size < required_size) // Provide multiple victims based on required size for lazy victim fetching
        {
            Key tmp_victim_key;
            ObjectSize tmp_victim_object_size = 0;
            Popularity tmp_local_cached_popularity = 0.0;
            Popularity tmp_redirected_cached_popularity = 0.0;
            Reward tmp_local_reward = 0.0;

            bool is_least_popular_key_exist = local_cached_metadata_.getLeastRewardKeyAndReward(least_popular_rank, tmp_victim_key, tmp_local_reward);

            if (is_least_popular_key_exist)
            {
                tmp_victim_object_size = local_cached_metadata_.getObjectSize(tmp_victim_key);
                local_cached_metadata_.getPopularity(tmp_victim_key, tmp_local_cached_popularity, tmp_redirected_cached_popularity);

                if (keys.find(tmp_victim_key) == keys.end())
                {
                    keys.insert(tmp_victim_key);

                    tommyds_object_t* tmp_victim_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &tmp_victim_key, hashForTommyds_(tmp_victim_key));
                    assert(tmp_victim_ptr != NULL); // Victim must be cached before eviction
                    uint32_t tmp_victim_value_size = tmp_victim_ptr->val.getValuesize();
                    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
                    // NOTE: tmp_victim_object_size got from key-level metadata is already accurate
                    assert(tmp_victim_object_size == (tmp_victim_key.getKeyLength() + tmp_victim_value_size));
                    #else
                    // NOTE: tmp_victim_object_size got from group-level metadata is approximate, which should be replaced with the accurate one from local edge cache
                    tmp_victim_object_size = tmp_victim_key.getKeyLength() + tmp_victim_value_size;
                    #endif

                    // Push victim cacheinfo
                    VictimCacheinfo tmp_victim_info(tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);
                    assert(tmp_victim_info.isComplete()); // NOTE: victim cacheinfos from local edge cache MUST be complete
                    victim_cacheinfos.push_back(tmp_victim_info); // Add to the tail of the list

                    // NOTE: we CANNOT use delta of internal_kvbytes_ as conservative_victim_total_size (still conservative due to NOT considering saved metadata space), as we have NOT really evicted the victims from covered_cache_ptr_ yet
                    conservative_victim_total_size += (tmp_victim_object_size); // Count key size and value size into victim total size (conservative as we do NOT count metadata cache size usage -> the actual saved space after eviction should be larger than conservative_victim_total_size and also required_size)
                }

                has_victim_key = true;
                least_popular_rank += 1;
            }
            else
            {
                std::ostringstream oss;
                oss << "least_popular_rank " << least_popular_rank << " has used up popularity information for local cached objects -> cannot make room for the required size (" << required_size << " bytes) under cache capacity (" << capacity_bytes_ << " bytes)";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);

                //break;
            }
        }

        return has_victim_key;
    }

    bool CoveredLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        // TODO: this function will be invoked at each placement neighbor node by the beacon node for cache placement

        assert(hasFineGrainedManagement());

        bool is_evict = false;

        // Get victim value
        tommyds_object_t* tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_search(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));

        if (tmp_obj_ptr != NULL) // Key exists
        {
            value = tmp_obj_ptr->val;

            // TMPDEBUG231201
            uint64_t internal_size = internal_kvbytes_;
            uint64_t local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
            uint64_t local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
            Util::dumpVariablesForDebug(instance_name_, 6, "[before evict] internal_size:", std::to_string(internal_size).c_str(), "local_cached_metadata_size:", std::to_string(local_cached_metadata_size).c_str(), "local_uncached_metadata_size:", std::to_string(local_uncached_metadata_size).c_str());

            // Remove the corresponding cache item
            tmp_obj_ptr = (tommyds_object_t *) tommy_hashdyn_remove(covered_cache_ptr_, tommyds_compare, &key, hashForTommyds_(key));
            assert(tmp_obj_ptr != NULL);
            delete tmp_obj_ptr;
            tmp_obj_ptr = NULL;

            // Update internal key-value pair size usage
            internal_kvbytes_ = Util::uint64Minus(internal_kvbytes_, (key.getKeyLength() + value.getValuesize()));

            // TMPDEBUG231201
            uint64_t after_internal_size = internal_kvbytes_;
            uint64_t after_local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
            uint64_t after_local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
            Util::dumpVariablesForDebug(instance_name_, 6, "[after evict] internal_size:", std::to_string(after_internal_size).c_str(), "local_cached_metadata_size:", std::to_string(after_local_cached_metadata_size).c_str(), "local_uncached_metadata_size:", std::to_string(after_local_uncached_metadata_size).c_str());

            // TMPDEBUG231201
            Util::dumpVariablesForDebug(instance_name_, 6, "victim key", key.getKeystr().c_str(), "is_neighbor_cached:", Util::toString(local_cached_metadata_.checkIsNeighborCachedForExistingKey(key)).c_str(), "local reward:", std::to_string(local_cached_metadata_.getLocalRewardForExistingKey(key)).c_str());

            // Remove from local cached metadata for eviction
            local_cached_metadata_.removeForExistingKey(key, value);

            is_evict = true;   
        }

        return is_evict;
    }

    void CoveredLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void CoveredLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, const void* func_param_ptr)
    {
        assert(func_param_ptr != NULL);

        if (func_name == UPDATE_IS_NEIGHBOR_CACHED_FLAG_FUNC_NAME)
        {
            const bool is_exist = local_cached_metadata_.isKeyExist(key);
            if (is_exist) // Enable/disable is_neighbor_cached for local cached metadata if key exists
            {
                const bool is_neighbor_cached = *((const bool*)func_param_ptr);
                local_cached_metadata_.updateIsNeighborCachedForExistingKey(key, is_neighbor_cached);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "updateLocalCacheMetadataInternal_() does NOT support func_name " << func_name;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        return;
    }

    uint64_t CoveredLocalCache::getSizeForCapacityInternal_() const
    {
        // NOTE: should NOT use cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize, which is usable cache size (i.e. capacity) instead of used size
        uint64_t internal_size = internal_kvbytes_;

        // Count cache size usage for local cached objects
        uint64_t local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
        internal_size = Util::uint64Add(internal_size, local_cached_metadata_size);

        // Count cache size usage for local uncached objects
        uint64_t local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
        internal_size = Util::uint64Add(internal_size, local_uncached_metadata_size);

        return internal_size;
    }

    void CoveredLocalCache::checkPointersInternal_() const
    {
        assert(covered_cache_ptr_ != NULL);
        return;
    }

    bool CoveredLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

    uint32_t CoveredLocalCache::hashForTommyds_(const Key& key) const
    {
        return tommy_hash_u32(0, key.getKeystr().c_str(), key.getKeyLength());
    }
}