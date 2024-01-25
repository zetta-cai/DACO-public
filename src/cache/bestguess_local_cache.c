#include "cache/bestguess_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string BestGuessLocalCache::kClassName("BestGuessLocalCache");

    BestGuessLocalCache::BestGuessLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes), edge_idx_(edge_idx)
    {
        // (A) Const variable

        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // (B) Non-const shared variables of local cached objects for eviction

        bestguess_cache_.clear();
        lookup_map_.clear();
        peredge_victim_vtime_.clear();
        cur_vtime_ = 0;
        size_ = 0;
    }

    BestGuessLocalCache::~BestGuessLocalCache()
    {
        bestguess_cache_.clear();
        lookup_map_.clear();
        peredge_victim_vtime_.clear();
    }

    const bool BestGuessLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool BestGuessLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        lookupmap_const_iterator_t lookup_map_const_iter = lookup_map_.find(key);
        bool is_cached = (lookup_map_const_iter != lookup_map_.end());

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool BestGuessLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        lookupmap_iterator_t lookup_map_iter = lookup_map_.find(key);
        bool is_local_cached = (lookup_map_iter != lookup_map_.end());
        if (is_local_cached)
        {
            list_iterator_t list_iter = lookup_map_iter->second;
            assert(list_iter != bestguess_cache_.end());

            // Get value
            value = list_iter->getValue();

            // Move BestGuessItem to the head of LRU list (NO need to update cache size usage due to no changes)
            // -> Remove original item from LRU list
            bestguess_cache_.erase(list_iter);
            // -> Insert new item into LRU list
            BestGuessItem tmp_item(key, value, cur_vtime_);
            bestguess_cache_.push_front(tmp_item);

            // Update key lookup map (NO need to update cache size usage due to no changes)
            lookup_map_iter->second = bestguess_cache_.begin();
        }

        // Update current virtual time
        cur_vtime_++;

        return is_local_cached;
    }

    bool BestGuessLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // Check is local cached
        lookupmap_iterator_t lookup_map_iter = lookup_map_.find(key);
        bool is_local_cached = (lookup_map_iter != lookup_map_.end());

        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            list_iterator_t list_iter = lookup_map_iter->second;
            assert(list_iter != bestguess_cache_.end());

            // Get original value
            Value original_value = list_iter->getValue();
            
            // Update with the latest value and move to the head of LRU list
            // -> Remove original item from LRU list
            bestguess_cache_.erase(list_iter);
            // -> Insert new item into LRU list
            BestGuessItem tmp_item(key, value, cur_vtime_);
            bestguess_cache_.push_front(tmp_item);

            // Update cache size usage for new value in LRU list
            if (value.getValuesize() > original_value.getValuesize())
            {
                size_ = Util::uint64Add(size_, (value.getValuesize() - original_value.getValuesize()));
            }
            else if (value.getValuesize() < original_value.getValuesize())
            {
                size_ = Util::uint64Minus(size_, (original_value.getValuesize() - value.getValuesize()));
            }

            // Update key lookup map (NO need to update cache size usage due to no changes)
            lookup_map_iter->second = bestguess_cache_.begin();

            is_successful = true;
        }

        // Update current virtual time
        cur_vtime_++;

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool BestGuessLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(key);
        UNUSED(value);
        
        // BestGuess will NEVER invoke this function for independent admission
        return false;
    }

    void BestGuessLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // ONLY for COVERED

        is_successful = false;

        // Check is local cached
        lookupmap_const_iterator_t lookup_map_const_iter = lookup_map_.find(key);
        bool is_local_cached = (lookup_map_const_iter != lookup_map_.end());

        if (is_local_cached)
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " is already local cached before admitLocalCacheInternal_(), which should NOT happen under atomicity of DirectoryTable in CooperationWrapeprBase";
            Util::dumpWarnMsg(instance_name_, oss.str());

            is_successful = true;
            return;
        }

        // Admit new item into LRU list
        BestGuessItem tmp_item(key, value, cur_vtime_);
        bestguess_cache_.push_front(tmp_item);

        // Update cache size usage for new item in LRU list
        size_ = Util::uint64Add(size_, tmp_item.getSizeForCapacity());

        // Update key lookup map
        lookup_map_.insert(std::make_pair(key, bestguess_cache_.begin()));

        // Update cache size usage for new lookup entry
        size_ = Util::uint64Add(size_, key.getKeyLength() + sizeof(list_iterator_t));

        // Update current virtual time (although admission is not a cache access, still increase cur_vtime_ to avoid the same vtime for concurrent admission notifications)
        cur_vtime_++;

        is_successful = true;

        return;
    }

    bool BestGuessLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY for COVERED

        bool has_victim_key = bestguess_cache_.size() > 0;
        if (has_victim_key)
        {
            Key tmp_victim_key = bestguess_cache_.back().getKey();
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool BestGuessLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
       assert(hasFineGrainedManagement());

        bool is_evict = false;

        lookupmap_const_iterator_t lookup_map_const_iter = lookup_map_.find(key);
        bool is_local_cached = (lookup_map_const_iter != lookup_map_.end());
        if (is_local_cached)
        {
            // Remove item from LRU list
            list_iterator_t list_iter = lookup_map_const_iter->second;
            assert(list_iter != bestguess_cache_.end());
            bestguess_cache_.erase(list_iter);

            // Update cache size usage for removed item
            size_ = Util::uint64Minus(size_, list_iter->getSizeForCapacity());

            // Remove entry from key lookup map
            lookup_map_.erase(lookup_map_const_iter);

            // Update cache size usage for removed lookup entry
            size_ = Util::uint64Minus(size_, key.getKeyLength() + sizeof(list_iterator_t));
        }

        return is_evict;
    }

    void BestGuessLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void BestGuessLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CustomFuncParamBase* func_param_ptr)
    {
        assert(func_param_ptr != NULL);

        if (func_name == GetLocalVictimVtimeFuncParam::FUNCNAME)
        {
            GetLocalVictimVtimeFuncParam* tmp_param_ptr = static_cast<GetLocalVictimVtimeFuncParam*>(func_param_ptr);
            getLocalVictimVtimeInternal_(tmp_param_ptr);            
        }
        else if (func_name == UpdateNeighborVictimVtimeParam::FUNCNAME)
        {
            UpdateNeighborVictimVtimeParam* tmp_param_ptr = static_cast<UpdateNeighborVictimVtimeParam*>(func_param_ptr);
            updateNeighborVictimVtimeInternal_(tmp_param_ptr);
        }
        else if (func_name == GetPlacementEdgeIdxParam::FUNCNAME)
        {
            GetPlacementEdgeIdxParam* tmp_param_ptr = static_cast<GetPlacementEdgeIdxParam*>(func_param_ptr);
            getPlacementEdgeIdxInternal_(tmp_param_ptr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        return;
    }

    void BestGuessLocalCache::getLocalVictimVtimeInternal_(GetLocalVictimVtimeFuncParam* func_param_ptr) const
    {
        assert(func_param_ptr != NULL);

        // Get local victim vtime
        uint64_t local_victim_vtime = 0; // NOTE: keep victim as 0 if BestGuess cache is empty, which has the largest priority for admission placement
        if (bestguess_cache_.size() > 0)
        {
            local_victim_vtime = bestguess_cache_.back().getVtime();
        }

        // Update function param
        func_param_ptr->setLocalVictimVtime(local_victim_vtime);

        return;
    }

    void BestGuessLocalCache::updateNeighborVictimVtimeInternal_(UpdateNeighborVictimVtimeParam* func_param_ptr)
    {
        assert(func_param_ptr != NULL);

        // Update neighbor victim vtime
        uint32_t neighbor_edge_idx = func_param_ptr->getNeighborEdgeIdx();
        assert(neighbor_edge_idx != edge_idx_);
        uint64_t neighbor_victim_vtime = func_param_ptr->getNeighborVictimVtime();
        std::unordered_map<uint32_t, uint64_t>::iterator peredge_victim_vtime_iter = peredge_victim_vtime_.find(neighbor_edge_idx);
        if (peredge_victim_vtime_iter == peredge_victim_vtime_.end())
        {
            peredge_victim_vtime_.insert(std::make_pair(neighbor_edge_idx, neighbor_victim_vtime));
        }
        else
        {
            peredge_victim_vtime_iter->second = neighbor_victim_vtime;
        }

        return;
    }

    void BestGuessLocalCache::getPlacementEdgeIdxInternal_(GetPlacementEdgeIdxParam* func_param_ptr) const
    {
        assert(func_param_ptr != NULL);

        // Choose current edge node as the placement edge node by default
        uint32_t placement_edge_idx = edge_idx_; // Current edge index
        GetLocalVictimVtimeFuncParam tmp_param;
        getLocalVictimVtimeInternal_(&tmp_param);
        uint64_t min_victim_vtime = tmp_param.getLocalVictimVtime(); // Local victim vtime in current edge node

        // Get placement edge idx under best-guess replacement policy (i.e., approximate global LRU)
        for (std::unordered_map<uint32_t, uint64_t>::const_iterator peredge_victim_vtime_const_iter = peredge_victim_vtime_.begin(); peredge_victim_vtime_const_iter != peredge_victim_vtime_.end(); peredge_victim_vtime_const_iter++)
        {
            uint32_t tmp_neighbor_edge_idx = peredge_victim_vtime_const_iter->first;
            assert(tmp_neighbor_edge_idx != edge_idx_);
            uint64_t tmp_neighbor_victim_vtime = peredge_victim_vtime_const_iter->second;
            if (tmp_neighbor_victim_vtime < min_victim_vtime)
            {
                min_victim_vtime = tmp_neighbor_victim_vtime;
                placement_edge_idx = tmp_neighbor_edge_idx;
            }
        }

        // Update function param
        func_param_ptr->setPlacementEdgeIdx(placement_edge_idx);
        UNUSED(min_victim_vtime);

        return;
    }

    uint64_t BestGuessLocalCache::getSizeForCapacityInternal_() const
    {
        return size_;
    }

    void BestGuessLocalCache::checkPointersInternal_() const
    {
        return;
    }

    bool BestGuessLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }
}