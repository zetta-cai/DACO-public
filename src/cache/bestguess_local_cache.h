/*
 * BestGuessLocalCache: local edge cache of BestGuess with best-guess replacement policy (refer to Section 4.3 in original paper).
 *
 * NOTE: we mainly implement best-guess replacement policy (i.e., approximate global LRU), yet still use DHT-based content discovery with unseparated cooperative caching (acceptable).
 * -> (i) Content discovery and cache separation is orthogonal with our problem on cache management policies, i.e., out of our scope.
 * -> (ii) Keep content discovery and cache separation the same as all other methods for fair comparison.
 * -> (iii) DHT-based content discovery with unseparated cooperative caching is better than original design in BestGuess, which over-estimates BestGuess performance instead of under-estimating.
 * ---> (iii-i) BestGuess originally uses hint-based content discovery, which is infeasible under geo-distributed edge settings -> (a) scalability issue: each edge node needs to maintain content directory of all cached objects for all other edge nodes, which incurs space overhead; (b) synchronization issue: each edge node needs to synchronize updated hints to all other edge nodes, which incurs extra network overhead; (c) consistency issue: given an edge node, hints of other edge nodes may not be consistent and accurate, which incurs extra cache miss rate -> DHT-based content discovery is better (scalable, no synchronization, and consistent).
 * ---> (iii-ii) Best Guess originally uses local-cooperative cache separation, which is infeasible under geo-distributed edge settings -> local cache consumes a part of cache capacity yet cannot be used to serve redirected requests from neighbor edge nodes, which degrades global cache hit rate and incurs extra edge-cloud access latency -> unseparated cooperative caching is better (no capacity waste and hence larger global hit rate).
 *
 * By Siyuan Sheng (2024.01.25).
 */

#ifndef BESTGUESS_LOCAL_CACHE_H
#define BESTGUESS_LOCAL_CACHE_H

#include <list> // std::list
#include <string>
#include <vector>

#include "cache/basic_cache_custom_func_param.h"

namespace covered
{
    // Forward declaration
    class BestGuessLocalCache;
}

#include "cache/bestguess/bestguess_item.h"
#include "cache/local_cache_base.h"

namespace covered
{
    class BestGuessLocalCache : public LocalCacheBase
    {
    public:
        BestGuessLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~BestGuessLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        typedef std::list<BestGuessItem>::iterator list_iterator_t;
        typedef std::list<BestGuessItem>::const_iterator list_const_iterator_t;
        typedef std::unordered_map<Key, list_iterator_t, KeyHasher>::iterator lookupmap_iterator_t;
        typedef std::unordered_map<Key, list_iterator_t, KeyHasher>::const_iterator lookupmap_const_iterator_t;

        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) override; // Return if key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        virtual void invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) override; // Invoke some method-specific function for local edge cache
        void updateNeighborVictimVtimeInternal_(UpdateNeighborVictimVtimeParam* func_param_ptr); // Update victim vtime of the given neighbor edge node

        virtual void invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const override; // Invoke some method-specific function for local edge cache
        void getLocalVictimVtimeInternal_(GetLocalVictimVtimeFuncParam* func_param_ptr) const; // Get victim vtime of current edge node
        void getPlacementEdgeIdxInternal_(GetPlacementEdgeIdxParam* func_param_ptr) const; // Get placement edge idx under best-guess replacement policy

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const override;
        
        // Member variables

        // (A) Const variable
        std::string instance_name_;
        uint32_t edge_idx_;

        // (B) Non-const shared variables
        mutable std::list<BestGuessItem> bestguess_cache_; // BestGuess cache (in LRU order)
        mutable std::unordered_map<Key, list_iterator_t, KeyHasher> lookup_map_; // Lookup map for BestGuess cache (key -> iterator in BestGuess cache)
        mutable std::unordered_map<uint32_t, uint64_t> peredge_victim_vtime_; // Corresponding to oldest block list in original paper for best-guess replacement
        mutable uint64_t cur_vtime_;
        mutable uint64_t size_; // Current size of BestGuess cache (in bytes)
    };
}

#endif