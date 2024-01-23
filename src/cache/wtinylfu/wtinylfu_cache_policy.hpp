/*
 * WTinylfuCachePolicy: refer to Algorithm Fig.5 in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c, yet directly reimplement in C++ due to simplicity of W-TinyLFU to escape the dependency on libcachesim and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + insufficient cache size usage calculation).
 *
 * NOTE: (i) TinyLFU is an admission control framework, which uses CBF to compare popularity between to-be-admited object and victim for admission or not if cache is full (always admit if cache is not full); (ii) SLRU is a fine-grained replacement policy, which uses multiple LRU lists to lookup with SLRU cool operations and admit/evict starting from the lowest LRU list; (iii) W-TinyLRU is a small LRU-based window cache + TinyLFU with SLRU as main cache; W-TinyLFU always admit into window cache no matter cache is full or not, while TinyLFU's admission control becomes a part of from-window-to-main eviction process (coarse-grained).
 * 
 * NOTE: we follow lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c to always admit objects into window cache when total cache is not full, which is equivalent to original W-TinyLFU that evicts objects from window cache into main cache when LRU is full <- the reason is that when total cache is not full (i.e., main cache is also not full), objects evicted from window cache will be always admited into main cache.
 * 
 * NOTE: lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c NOT invoke LRU/SLRU cache_get_base() and hence NOT trigger LRU/SLRU's internal eviction even if LRU/SLRU is full -> TinyLFU's admission control will be triggered during from-window-to-main eviction when total cache is full.
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: NO need to use optimized data structures, as system bottleneck is network propagation latency in geo-distributed edge settings.
 * 
 * By Siyuan Sheng (2024.01.23).
 */

#ifndef WTINYLFU_CACHE_POLICY_HPP
#define WTINYLFU_CACHE_POLICY_HPP

#include <list>
#include <unordered_map>

#include "cache/slru/slru_cache_policy.hpp"
#include "common/util.h"

// NOTE: default settings refer to lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c
#define WTINYLFU_WINDOW_RATIO 0.01
#define WTINYLFU_CBF_RATIO 0.001

namespace covered
{
    template <typename Key, typename Value>
    class WTinylfuItem
    {
    public:
        WTinylfuItem(): key_(), value_()
        {
        }

        WTinylfuItem(const Key& key, const Value& value)
        {
            key_ = key;
            value_ = value;
        }

        Key getKey() const
        {
            return key_;
        }

        Value getValue() const
        {
            return value_;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(key_.getKeyLength() + value_.getValuesize());
        }

        const WTinylfuItem<Key, Value>& operator=(const WTinylfuItem<Key, Value>& other)
        {
            if (this != &other)
            {
                key_ = other.key_;
                value_ = other.value_;
            }
            return *this;
        }
    private:
        Key key_;
        Value value_;
    };

    template <typename Key, typename Value, typename KeyHasher>
    class WTinylfuCachePolicy
    {
    public:
        typedef typename std::list<WTinylfuItem<Key, Value>>::iterator windowlist_iterator_t;
        typedef typename std::list<WTinylfuItem<Key, Value>>::const_iterator windowlist_const_iterator_t;
        typedef typename std::unordered_map<Key, windowlist_iterator_t, KeyHasher>::iterator lookupmap_iterator_t;
        typedef typename std::unordered_map<Key, windowlist_iterator_t, KeyHasher>::const_iterator lookupmap_const_iterator_t;
    private:
        // Function declarations
        // Cache eviction
        bool access_(const Key& key, Value& fetched_value, const bool& is_update = false, const Value& new_value = Value());
        bool _ARC_to_replace(Key& key);
        bool _ARC_to_evict_miss_on_all_queues(Key& key);
        void _ARC_evict_L1_data_no_ghost(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value);
        void _ARC_evict_L1_data(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value);
        void _ARC_evict_L1_ghost();
        void _ARC_evict_L2_data(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value);
        void _ARC_evict_L2_ghost();
        // Cache size calculation
        void decreaseGhostAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id);
        void increaseGhostAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id);
        void decreaseDataAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id);
        void increaseDataAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id);
        void decreaseTotalSize_(const uint64_t& decrease_bytes);
        void increaseTotalSize_(const uint64_t& increase_bytes);
    public:
        WTinylfuCachePolicy(const uint64_t& capacity_bytes) //: capacity_bytes_(capacity_bytes)
        {
            uint64_t main_cache_size = capacity_bytes * (1.0 - WTINYLFU_WINDOW_RATIO - WTINYLFU_CBF_RATIO);

            window_cache_.clear();
            main_cache_ptr_ = new SlruCachePolicy<Key, Value, KeyHasher>(main_cache_size);
            assert(main_cache_ptr_ != NULL);
            cbf_decay_threshold_ = 32 * main_cache_size;
            request_bytes_for_main_ = 0;

            cbf_ = (struct minimalIncrementCBF *)malloc(sizeof(struct minimalIncrementCBF));
            assert(cbf_ != NULL);
            cbf_->ready = 0;
            int ret = minimalIncrementCBF_init(cbf_, main_cache_size, WTINYLFU_CBF_RATIO);
            if (ret != 0) {
                Util::dumpErrorMsg(kClassName, "CBF init failed!");
                exit(1);
            }

            key_lookup_.clear();
            size_ = 0;
        }

        ~WTinylfuCachePolicy()
        {
            window_cache_.clear();

            assert(main_cache_ptr_ != NULL);
            delete main_cache_ptr_;
            main_cache_ptr_ = NULL;

            assert(cbf_ != NULL);
            delete cbf_;
            cbf_ = NULL;
            
            key_lookup_.clear();
        }

        bool exists(const Key& key) const
        {
            // TODO: END HERE

            bool is_local_cached = false;

            lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_.find(key);
            if (lookup_map_const_iter != key_lookup_.end() && !lookup_map_const_iter->second.isGhost()) // Key exists in L1/L2 data list
            {
                is_local_cached = true;
            }

            return is_local_cached;
        }

        bool get(const Key& key, Value& value)
        {
            bool is_local_cached = access_(key, value);
            return is_local_cached;
        }

        bool update(const Key& key, const Value& value)
        {
            Value unused_fetched_value;
            bool is_local_cached = access_(key, unused_fetched_value, true, value);

            return is_local_cached;
        }

        void admit(const Key& key, const Value& value)
        {
            lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_.find(key);
            if (lookup_map_const_iter != key_lookup_.end()) // Previous list and map entry exist
            {
                // NOTE: object MUST be in L1/L2 data list, as ghost list entry will be removed if any in access_() for cache miss before admit()
                assert(!lookup_map_const_iter->second.isGhost());

                std::ostringstream oss;
                oss << "key " << key.getKeystr() << " already exists in key_lookup_ (L1 data list size: " << L1_data_list_.size() << "; L2 data list size: " << L2_data_list_.size() << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                if (vtime_last_req_in_ghost_ == reqseq_ &&
                    (curr_obj_in_L1_ghost_ || curr_obj_in_L2_ghost_)) {
                    // insert to L2 data head
                    ArcDataItem tmp_item(key, value, 2);
                    L2_data_list_.push_front(tmp_item);
                    increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), 2);

                    curr_obj_in_L1_ghost_ = false;
                    curr_obj_in_L2_ghost_ = false;
                    vtime_last_req_in_ghost_ = -1;

                    // Insert into key lookup map
                    ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L2_data_list_.begin());
                    key_lookup_.insert(std::pair(key, tmp_lookup_iter));
                    increaseTotalSize_(key.getKeyLength() + tmp_lookup_iter.getSizeForCapacity());
                } else {
                    // insert to L1 data head
                    ArcDataItem<Key, Value> tmp_item(key, value, 1);
                    L1_data_list_.push_front(tmp_item);
                    increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), 1);

                    // Insert into key lookup map
                    ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L1_data_list_.begin());
                    key_lookup_.insert(std::pair(key, tmp_lookup_iter));
                    increaseTotalSize_(key.getKeyLength() + tmp_lookup_iter.getSizeForCapacity());
                }

                incoming_objsize_ = key.getKeyLength() + value.getValuesize(); // Siyuan: update the most recent admited object size for potential eviction
            }

            return;
        }

        bool getVictimKey(Key& key)
        {
            bool has_victim_key = false;

            to_evict_candidate_gen_vtime_ = reqseq_;
            if (vtime_last_req_in_ghost_ == reqseq_ &&
                (curr_obj_in_L1_ghost_ || curr_obj_in_L2_ghost_)) {
                has_victim_key = _ARC_to_replace(key);
            } else {
                has_victim_key = _ARC_to_evict_miss_on_all_queues(key);
            }

            return has_victim_key;
        }

        bool evictWithGivenKey(const Key& key, Value& value)
        {
            bool is_evict = false;
            
            lookupmap_iterator_t lookup_map_iter = key_lookup_.find(key);
            if (lookup_map_iter != key_lookup_.end()) {
                // NOTE: getVictimKey() MUST get a victim from L1/L2 data list instead of any ghost list
                assert(!lookup_map_iter->second.isGhost());

                int lru_id = lookup_map_iter->second.getLruId();
                if (lru_id == 1) {
                    if (L1_data_size_ + L1_ghost_size_ + incoming_objsize_ > capacity_bytes_) {
                        // case A: L1 = T1 U B1 has exactly c pages
                        if (L1_ghost_size_ > 0) {
                            // if T1 < c (ghost is not empty),
                            // delete the LRU of the L1 ghost, and replace
                            // we do not use params->L1_data_size < cache->cache_size
                            // because it does not work for variable size objects
                            _ARC_evict_L1_ghost();
                        } else {
                            // T1 >= c, L1 data size is too large, ghost is empty, so evict from L1
                            // data
                            _ARC_evict_L1_data_no_ghost(lookup_map_iter, key, value); // Get value, remove from data LRU 1, and remove from key lookup map (NOTE: NOT insert into ghost LRU 1)
                            is_evict = true;

                            return is_evict;
                        }
                    }

                    _ARC_evict_L1_data(lookup_map_iter, key, value); // Get value, remove from data LRU 1, insert into ghost LRU 1, and update key lookup map
                    is_evict = true;
                }
                else if (lru_id == 2) {
                    if (L1_data_size_ + L1_ghost_size_ + L2_data_size_ + L2_ghost_size_ >= capacity_bytes_ * 2) {
                        // delete the LRU end of the L2 ghost
                        if (L2_ghost_size_ > 0) {
                            // it maybe empty if object size is variable
                            _ARC_evict_L2_ghost();
                        }
                    }

                    _ARC_evict_L2_data(lookup_map_iter, key, value); // Get value, remove from data LRU 2, insert into ghost LRU 2, and update key lookup map
                    is_evict = true;
                }
                else {
                    std::ostringstream oss;
                    oss << "invalid lru_id " << lru_id;
                    Util::dumpErrorMsg(kClassName, oss.str());
                    exit(1);
                }
            }

            return is_evict;
        }

        uint64_t getSizeForCapacity() const
        {
            return size_;
        }
    private:
        static const std::string kClassName;

        std::list<WTinylfuItem<Key, Value>> window_cache_; // LRU as windowed LRU
        SlruCachePolicy<Key, Value, KeyHasher>* main_cache_ptr_; // SLRU as main cache
        struct minimalIncrementCBF *cbf_;
        uint64_t cbf_decay_threshold_; // Siyuan: corresponding to max_request_num in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c yet in units of bytes instead of number of requests
        uint64_t request_bytes_for_main_; // Siyuan: corresponding to request_counter in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c yet in units of bytes instead of number of requests
        //request_t *req_local; // Siyuan: just used as a temporary variable to track victim object in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c

        std::unordered_map<Key, windowlist_iterator_t, KeyHasher> key_lookup_for_window_; // Key indexing for quick lookup of window cache (main cache lookup is supported by SlruCachePolicy itself)
        uint64_t size_; // in units of bytes
        //const uint64_t capacity_bytes_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string ArcCachePolicy<Key, Value, KeyHasher>::kClassName("ArcCachePolicy");

    // Cache access for get and update

    template <typename Key, typename Value, typename KeyHasher>
    bool ArcCachePolicy<Key, Value, KeyHasher>::access_(const Key& key, Value& fetched_value, const bool& is_update, const Value& new_value)
    {
        reqseq_ += 1; // Update global virtual/logical timestamp

        bool is_local_cached = false;

        lookupmap_iterator_t lookup_map_iter = key_lookup_.find(key);
        if (lookup_map_iter == key_lookup_.end())
        {
            is_local_cached = false;
        }
        else
        {
            curr_obj_in_L1_ghost_ = false;
            curr_obj_in_L2_ghost_ = false;

            int lru_id = lookup_map_iter->second.getLruId();
            if (lookup_map_iter->second.isGhost())
            {
                is_local_cached = false;

                // ghost hit
                vtime_last_req_in_ghost_ = reqseq_;
                if (lru_id == 1) {
                    const ghostlist_const_iterator_t& ghost_list_const_iter_const_ref = lookup_map_iter->second.getGhostListIterConstRef();
                    assert(ghost_list_const_iter_const_ref != L1_ghost_list_.end());
                    assert(ghost_list_const_iter_const_ref->getKey() == key);
                    assert(ghost_list_const_iter_const_ref->getLruId() == lru_id);
                    assert(ghost_list_const_iter_const_ref->isGhost());

                    curr_obj_in_L1_ghost_ = true;
                    // case II: x in L1_ghost
                    assert(L1_ghost_size_ >= 1);
                    double delta = MAX((double)L2_ghost_size_ / L1_ghost_size_, 1);
                    p_ = MIN(p_ + delta, capacity_bytes_);

                    // Remove ArcGhostItem from ghost LRU 1
                    decreaseGhostAndTotalSize_(ghost_list_const_iter_const_ref->getSizeForCapacity(), lru_id);
                    L1_ghost_list_.erase(ghost_list_const_iter_const_ref);
                }
                else if (lru_id == 2) {
                    const ghostlist_const_iterator_t& ghost_list_const_iter_const_ref = lookup_map_iter->second.getGhostListIterConstRef();
                    assert(ghost_list_const_iter_const_ref != L2_ghost_list_.end());
                    assert(ghost_list_const_iter_const_ref->getKey() == key);
                    assert(ghost_list_const_iter_const_ref->getLruId() == lru_id);
                    assert(ghost_list_const_iter_const_ref->isGhost());

                    curr_obj_in_L2_ghost_ = true;
                    // case III: x in L2_ghost
                    assert(L2_ghost_size_ >= 1);
                    double delta = MAX((double)L1_ghost_size_ / L2_ghost_size_, 1);
                    p_ = MAX(p_ - delta, 0);

                    // Remove ArcGhostItem from ghost LRU 2
                    decreaseGhostAndTotalSize_(ghost_list_const_iter_const_ref->getSizeForCapacity(), lru_id);
                    L2_ghost_list_.erase(ghost_list_const_iter_const_ref);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid lru_id " << lru_id;
                    Util::dumpErrorMsg(kClassName, oss.str());
                    exit(1);
                }

                // Remove lookup map entry
                decreaseTotalSize_(key.getKeyLength() + lookup_map_iter->second.getSizeForCapacity());
                key_lookup_.erase(lookup_map_iter);
            }
            else
            {
                is_local_cached = true;

                // cache hit, case I: x in L1_data or L2_data
                if (lru_id == 1) {
                    const datalist_iterator_t& data_list_iter_const_ref = lookup_map_iter->second.getDataListIterConstRef();
                    assert(data_list_iter_const_ref != L1_data_list_.end());
                    assert(data_list_iter_const_ref->getKey() == key);
                    assert(data_list_iter_const_ref->getLruId() == lru_id);
                    assert(!data_list_iter_const_ref->isGhost());

                    // Get value
                    fetched_value = data_list_iter_const_ref->getValue();

                    // move to LRU2 (need to update cache size usage especially for in-place update)
                    // Remove from data LRU 1
                    decreaseDataAndTotalSize_(data_list_iter_const_ref->getSizeForCapacity(), lru_id);
                    L1_data_list_.erase(data_list_iter_const_ref);
                    // Insert into data LRU 2
                    Value tmp_value = is_update ? new_value : fetched_value;
                    ArcDataItem tmp_item(key, tmp_value, 2);
                    increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), 2);
                    L2_data_list_.push_front(tmp_item);

                    // update lookup table (no need to update cache size usage due to no change)
                    ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L2_data_list_.begin());
                    assert(tmp_lookup_iter.getLruId() == 2);
                    assert(!tmp_lookup_iter.isGhost());
                    lookup_map_iter->second = tmp_lookup_iter;
                } else {
                    const datalist_iterator_t& data_list_iter_const_ref = lookup_map_iter->second.getDataListIterConstRef();
                    assert(data_list_iter_const_ref != L2_data_list_.end());
                    assert(data_list_iter_const_ref->getKey() == key);
                    assert(data_list_iter_const_ref->getLruId() == lru_id);
                    assert(!data_list_iter_const_ref->isGhost());

                    // Get value
                    fetched_value = data_list_iter_const_ref->getValue();

                    // move to LRU2 head (need to update cache size usage especially for in-place update)
                    // Remove from data LRU 2
                    decreaseDataAndTotalSize_(data_list_iter_const_ref->getSizeForCapacity(), lru_id);
                    L2_data_list_.erase(data_list_iter_const_ref);
                    // Insert into head of data LRU 2
                    Value tmp_value = is_update ? new_value : fetched_value;
                    ArcDataItem tmp_item(key, tmp_value, 2);
                    increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), lru_id);
                    L2_data_list_.push_front(tmp_item);

                    // update lookup table (no need to update cache size usage due to no change)
                    ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L2_data_list_.begin());
                    assert(tmp_lookup_iter.getLruId() == 2);
                    assert(!tmp_lookup_iter.isGhost());
                    lookup_map_iter->second = tmp_lookup_iter;
                }
            }
        }

        return is_local_cached;
    }

    // Cache eviction

    /* finding the eviction candidate in _ARC_replace but do not perform eviction */
    template <typename Key, typename Value, typename KeyHasher>
    bool ArcCachePolicy<Key, Value, KeyHasher>::_ARC_to_replace(Key& key)
    {
        bool has_victim_key = false;

        if (L1_data_size_ == 0 && L2_data_size_ == 0) {
            std::ostringstream oss;
            oss << "L1_data_size_ == 0 && L2_data_size_ == 0";
            Util::dumpWarnMsg(kClassName, oss.str());
            return has_victim_key;
        }

        bool cond1 = L1_data_size_ > 0;
        bool cond2 = L1_data_size_ > p_;
        bool cond3 = L1_data_size_ == p_ && curr_obj_in_L2_ghost_;
        bool cond4 = L2_data_size_ == 0;

        if ((cond1 && (cond2 || cond3)) || cond4) {
            // delete the LRU in L1 data, move to L1_ghost
            key = L1_data_list_.back().getKey();
            has_victim_key = true;
        } else {
            // delete the item in L2 data, move to L2_ghost
            key = L2_data_list_.back().getKey();
            has_victim_key = true;
        }

        assert(key_lookup_.find(key) != key_lookup_.end());
        return has_victim_key;
    }

    /* finding the eviction candidate in _ARC_evict_miss_on_all_queues, but do not
    * perform eviction */
    template <typename Key, typename Value, typename KeyHasher>
    bool ArcCachePolicy<Key, Value, KeyHasher>::_ARC_to_evict_miss_on_all_queues(Key& key)
    {
        bool has_victim_key = false;

        if (L1_data_size_ + L1_ghost_size_ + incoming_objsize_ > capacity_bytes_)
        {
            // case A: L1 = T1 U B1 has exactly c pages
            if (L1_ghost_size_ > 0) {
                has_victim_key = _ARC_to_replace(key);
            } else {
            // T1 >= c, L1 data size is too large, ghost is empty, so evict from L1
            // data
                key = L1_data_list_.back().getKey();
                has_victim_key = true;
            }
        } else {
            has_victim_key = _ARC_to_replace(key);
        }

        return has_victim_key;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::_ARC_evict_L1_data_no_ghost(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value)
    {
        assert(lookupmap_iter_const_ref != key_lookup_.end());
        const datalist_const_iterator_t& data_list_const_iter_const_ref = lookupmap_iter_const_ref->second.getDataListIterConstRef();

        assert(data_list_const_iter_const_ref != L1_data_list_.end());
        assert(data_list_const_iter_const_ref->getKey() == key);
        assert(data_list_const_iter_const_ref->getLruId() == 1);
        assert(!data_list_const_iter_const_ref->isGhost());

        // Get value
        value = data_list_const_iter_const_ref->getValue();

        // Remove from data LRU 1
        decreaseDataAndTotalSize_(data_list_const_iter_const_ref->getSizeForCapacity(), 1);
        L1_data_list_.erase(data_list_const_iter_const_ref);

        // Remove from key lookup map
        decreaseTotalSize_(key.getKeyLength() + lookupmap_iter_const_ref->second.getSizeForCapacity());
        key_lookup_.erase(lookupmap_iter_const_ref);

        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::_ARC_evict_L1_data(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value)
    {
        assert(lookupmap_iter_const_ref != key_lookup_.end());
        const datalist_const_iterator_t& data_list_const_iter_const_ref = lookupmap_iter_const_ref->second.getDataListIterConstRef();

        assert(data_list_const_iter_const_ref != L1_data_list_.end());
        assert(data_list_const_iter_const_ref->getKey() == key);
        assert(data_list_const_iter_const_ref->getLruId() == 1);
        assert(!data_list_const_iter_const_ref->isGhost());

        // Get value
        value = data_list_const_iter_const_ref->getValue();

        // Remove from data LRU 1
        decreaseDataAndTotalSize_(data_list_const_iter_const_ref->getSizeForCapacity(), 1);
        L1_data_list_.erase(data_list_const_iter_const_ref);

        // Insert into ghost LRU 1
        ArcGhostItem<Key, Value> tmp_item(key, 1);
        increaseGhostAndTotalSize_(tmp_item.getSizeForCapacity(), 1);
        L1_ghost_list_.push_front(tmp_item);

        // Update key lookup map (no need to update cache size usage due to no change)
        ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L1_ghost_list_.begin());
        assert(tmp_lookup_iter.getLruId() == 1);
        assert(tmp_lookup_iter.isGhost());
        lookupmap_iter_const_ref->second = tmp_lookup_iter;

        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::_ARC_evict_L1_ghost()
    {
        assert(L1_ghost_list_.size() > 0);
        ghostlist_const_iterator_t ghost_list_const_iter = L1_ghost_list_.end();
        ghost_list_const_iter--;

        assert(ghost_list_const_iter != L1_ghost_list_.end());
        assert(ghost_list_const_iter->getLruId() == 1);
        assert(ghost_list_const_iter->isGhost());

        Key tmp_key = ghost_list_const_iter->getKey();

        // Remove from ghost LRU 1
        decreaseGhostAndTotalSize_(ghost_list_const_iter->getSizeForCapacity(), 1);
        L1_ghost_list_.erase(ghost_list_const_iter);

        // Remove from key lookup map
        lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_.find(tmp_key);
        assert(lookup_map_const_iter != key_lookup_.end());
        assert(lookup_map_const_iter->second.getLruId() == 1);
        assert(lookup_map_const_iter->second.isGhost());
        decreaseTotalSize_(tmp_key.getKeyLength() + lookup_map_const_iter->second.getSizeForCapacity());
        key_lookup_.erase(lookup_map_const_iter);

        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::_ARC_evict_L2_data(const lookupmap_iterator_t& lookupmap_iter_const_ref, const Key& key, Value& value)
    {
        assert(lookupmap_iter_const_ref != key_lookup_.end());
        const datalist_const_iterator_t& data_list_const_iter_const_ref = lookupmap_iter_const_ref->second.getDataListIterConstRef();

        assert(data_list_const_iter_const_ref != L2_data_list_.end());
        assert(data_list_const_iter_const_ref->getKey() == key);
        assert(data_list_const_iter_const_ref->getLruId() == 2);
        assert(!data_list_const_iter_const_ref->isGhost());

        // Get value
        value = data_list_const_iter_const_ref->getValue();

        // Remove from data LRU 2
        decreaseDataAndTotalSize_(data_list_const_iter_const_ref->getSizeForCapacity(), 2);
        L2_data_list_.erase(data_list_const_iter_const_ref);

        // Insert into ghost LRU 2
        ArcGhostItem<Key, Value> tmp_item(key, 2);
        increaseGhostAndTotalSize_(tmp_item.getSizeForCapacity(), 2);
        L2_ghost_list_.push_front(tmp_item);

        // Update key lookup map (no need to update cache size usage due to no change)
        ArcKeyLookupIter<Key, Value> tmp_lookup_iter(L2_ghost_list_.begin());
        assert(tmp_lookup_iter.getLruId() == 2);
        assert(tmp_lookup_iter.isGhost());
        lookupmap_iter_const_ref->second = tmp_lookup_iter;

        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::_ARC_evict_L2_ghost()
    {
        assert(L2_ghost_list_.size() > 0);
        ghostlist_const_iterator_t ghost_list_const_iter = L2_ghost_list_.end();
        ghost_list_const_iter--;

        assert(ghost_list_const_iter != L2_ghost_list_.end());
        assert(ghost_list_const_iter->getLruId() == 2);
        assert(ghost_list_const_iter->isGhost());

        Key tmp_key = ghost_list_const_iter->getKey();

        // Remove from ghost LRU 2
        decreaseGhostAndTotalSize_(ghost_list_const_iter->getSizeForCapacity(), 2);
        L2_ghost_list_.erase(ghost_list_const_iter);

        // Remove from key lookup map
        lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_.find(tmp_key);
        assert(lookup_map_const_iter != key_lookup_.end());
        assert(lookup_map_const_iter->second.getLruId() == 2);
        assert(lookup_map_const_iter->second.isGhost());
        decreaseTotalSize_(tmp_key.getKeyLength() + lookup_map_const_iter->second.getSizeForCapacity());
        key_lookup_.erase(lookup_map_const_iter);

        return;
    }

    // Cache size usage calculation

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::decreaseGhostAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id)
    {
        assert(lru_id == 1 || lru_id == 2);

        if (lru_id == 1) // Ghost LRU 1
        {
            if (L1_ghost_size_ <= decrease_bytes)
            {
                L1_ghost_size_ = 0;
            }
            else
            {
                L1_ghost_size_ -= decrease_bytes;
            }
        }
        else if (lru_id == 2) // Ghost LRU 2
        {
            if (L2_ghost_size_ <= decrease_bytes)
            {
                L2_ghost_size_ = 0;
            }
            else
            {
                L2_ghost_size_ -= decrease_bytes;
            }
        }

        decreaseTotalSize_(decrease_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::increaseGhostAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id)
    {
        assert(lru_id == 1 || lru_id == 2);

        if (lru_id == 1) // Ghost LRU 1
        {
            L1_ghost_size_ += increase_bytes;
        }
        else // Ghost LRU 2
        {
            L2_ghost_size_ += increase_bytes;
        }

        increaseTotalSize_(increase_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::decreaseDataAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id)
    {
        assert(lru_id == 1 || lru_id == 2);

        if (lru_id == 1) // Data LRU 1
        {
            if (L1_data_size_ <= decrease_bytes)
            {
                L1_data_size_ = 0;
            }
            else
            {
                L1_data_size_ -= decrease_bytes;
            }
        }
        else // Data LRU 2
        {
            if (L2_data_size_ <= decrease_bytes)
            {
                L2_data_size_ = 0;
            }
            else
            {
                L2_data_size_ -= decrease_bytes;
            }
        }

        decreaseTotalSize_(decrease_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::increaseDataAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id)
    {
        assert(lru_id == 1 || lru_id == 2);

        if (lru_id == 1) // Data LRU 1
        {
            L1_data_size_ += increase_bytes;
        }
        else // Data LRU 2
        {
            L2_data_size_ += increase_bytes;
        }

        increaseTotalSize_(increase_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::decreaseTotalSize_(const uint64_t& decrease_bytes)
    {
        if (size_ <= decrease_bytes)
        {
            size_ = 0;
        }
        else
        {
            size_ -= decrease_bytes;
        }
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void ArcCachePolicy<Key, Value, KeyHasher>::increaseTotalSize_(const uint64_t& increase_bytes)
    {
        size_ += increase_bytes;
        return;
    }

} // namespace covered

#endif // ARC_CACHE_POLICY_HPP
