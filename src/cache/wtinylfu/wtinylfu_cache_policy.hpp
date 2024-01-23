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
#include "common/cbf.h"
#include "common/util.h"

// NOTE: default settings refer to lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c
#define WTINYLFU_WINDOW_RATIO 0.01
#define WTINYLFU_CBF_ERROR 0.001

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

        void setValue(const Value& value)
        {
            value_ = value;
            return;
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
        // Cache access for get and update
        bool access_(const Key& key, Value& fetched_value, const bool& is_update = false, const Value& new_value = Value());
        // Cache size calculation
        void decreaseWindowSize_(const uint64_t& decrease_bytes);
        void increaseWindowSize_(const uint64_t& increase_bytes);
    public:
        WTinylfuCachePolicy(const uint64_t& capacity_bytes) //: capacity_bytes_(capacity_bytes)
        {
            uint64_t main_cache_size = capacity_bytes * (1.0 - WTINYLFU_WINDOW_RATIO);

            // Siyuan: fix impractical input of max # of objects
            const uint64_t entries = main_cache_size / 10 / 1024; // Siyuan: assume average object size is in units of 10 KiB (e.g., average object size is ~30 KiB in Facebook CDN trace)
            cbf_ = new CBF(entries, WTINYLFU_CBF_ERROR);
            assert(cbf_ != NULL);
            const uint64_t cbf_size = cbf_->getSizeForCapacity();
            assert(cbf_size < main_cache_size);
            main_cache_size -= cbf_size;

            window_cache_.clear();
            main_cache_ptr_ = new SlruCachePolicy<Key, Value, KeyHasher>(main_cache_size);
            assert(main_cache_ptr_ != NULL);
            cbf_decay_threshold_ = 32 * main_cache_size;
            request_bytes_for_main_ = 0;

            key_lookup_.clear();
            window_size_ = 0;
        }

        ~WTinylfuCachePolicy()
        {
            assert(cbf_ != NULL);
            delete cbf_;
            cbf_ = NULL;

            window_cache_.clear();

            assert(main_cache_ptr_ != NULL);
            delete main_cache_ptr_;
            main_cache_ptr_ = NULL;
            
            key_lookup_.clear();
        }

        bool exists(const Key& key) const
        {
            bool is_local_cached = false;

            // Find in window cache
            lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_for_window_.find(key);
            if (lookup_map_const_iter != key_lookup_for_window_.end()) // Key exists in window cache
            {
                is_local_cached = true;
            }

            // Find in main cache
            if (!is_local_cached)
            {
                is_local_cached = main_cache_ptr_->exists(key);
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
            return window_size_ + cbf_->getSizeForCapacity() + main_cache_ptr_->getSizeForCapacity();
        }
    private:
        static const std::string kClassName;

        CBF* cbf_;
        std::list<WTinylfuItem<Key, Value>> window_cache_; // LRU as windowed LRU
        SlruCachePolicy<Key, Value, KeyHasher>* main_cache_ptr_; // SLRU as main cache
        uint64_t cbf_decay_threshold_; // Siyuan: corresponding to max_request_num in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c yet in units of bytes instead of number of requests (due to cache capacity restriction in units of bytes)
        uint64_t request_bytes_for_main_; // Siyuan: corresponding to request_counter in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c yet in units of bytes instead of number of requests (due to cache capacity restriction in units of bytes)
        //request_t *req_local; // Siyuan: just used as a temporary variable to track victim object in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/WTinyLFU.c

        std::unordered_map<Key, windowlist_iterator_t, KeyHasher> key_lookup_for_window_; // Key indexing for quick lookup of window cache (main cache lookup is supported by SlruCachePolicy itself)
        uint64_t window_size_; // Cache size usage of window cache and lookup table in units of bytes
        //const uint64_t capacity_bytes_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string WTinylfuCachePolicy<Key, Value, KeyHasher>::kClassName("WTinylfuCachePolicy");

    // Cache access for get and update

    template <typename Key, typename Value, typename KeyHasher>
    bool WTinylfuCachePolicy<Key, Value, KeyHasher>::access_(const Key& key, Value& fetched_value, const bool& is_update, const Value& new_value)
    {
        bool is_local_cached = false;

        // Access window cache
        bool is_local_cached_in_window = false;
        Value fetched_value_in_window;
        lookupmap_iterator_t lookup_map_iter = key_lookup_for_window_.find(key);
        if (lookup_map_iter != key_lookup_for_window_.end())
        {
            is_local_cached_in_window = true;

            // Get value
            fetched_value_in_window = lookup_map_iter->second->getValue();

            // Update value if necessary
            if (is_update)
            {
                lookup_map_iter->second->setValue(new_value);

                // Update cache size usage (TODO: END HERE)
                decreaseWindowSize_()
            }
        }

        // Access main cache
        Value fetched_value_in_main;
        bool is_local_cached_in_main = main_cache_ptr_->get(key, fetched_value_in_main); // Get value
        if (is_local_cached_in_main && is_update)
        {
            // Update value if necessary
            bool tmp_is_local_cached_in_main = main_cache_ptr_->update(key, new_value); // TODO: END HERE
            assert(tmp_is_local_cached_in_main);
        }

        // Get value if cached in window/main cache
        is_local_cached = is_local_cached_in_window || is_local_cached_in_main;
        if (is_local_cached)
        {
            fetched_value = is_local_cached_in_window ? fetched_value_in_window : fetched_value_in_main;
        }

        // Update CBF if necessary
        if (is_local_cached_in_main)
        {
            // frequency update
            cbf_->update(key);

            // Update request bytes
            const uint64_t tmp_value_bytes = is_update ? new_value.getValuesize() : fetched_value.getValuesize();
            request_bytes_for_main_ = key.getKeyLength() + tmp_value_bytes;

            // Decay CBF if necessary
            if (request_bytes_for_main_ >= cbf_decay_threshold_) {
                request_bytes_for_main_ = 0;
                cbf_->decay();
            }
        }

        return is_local_cached;
    }

    // Cache size usage calculation

    template <typename Key, typename Value, typename KeyHasher>
    void WTinylfuCachePolicy<Key, Value, KeyHasher>::decreaseWindowSize_(const uint64_t& decrease_bytes)
    {
        if (window_size_ <= decrease_bytes)
        {
            window_size_ = 0;
        }
        else
        {
            window_size_ -= decrease_bytes;
        }
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void WTinylfuCachePolicy<Key, Value, KeyHasher>::increaseWindowSize_(const uint64_t& increase_bytes)
    {
        window_size_ += increase_bytes;
        return;
    }

} // namespace covered

#endif // ARC_CACHE_POLICY_HPP
