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

            cbf_max_bytes_ = cbf_size;
            window_cache_max_bytes_ = capacity_bytes * WTINYLFU_WINDOW_RATIO;
            main_cache_max_bytes_ = main_cache_size;

            key_lookup_for_window_.clear();
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
            
            key_lookup_for_window_.clear();
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
            UNUSED(unused_fetched_value);

            return is_local_cached;
        }

        bool canAdmit(const uint32_t& objsize) const
        {
            return objsize <= window_cache_max_bytes_ && main_cache_ptr_->canAdmit(objsize); // Need <= window cache size and main cache size
        }

        void admit(const Key& key, const Value& value)
        {
            // Check window cache
            lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_for_window_.find(key);
            if (lookup_map_const_iter != key_lookup_for_window_.end()) // Previous list and map entry exist
            {
                std::ostringstream oss;
                oss << "key " << key.getKeyDebugstr() << " already exists in key_lookup_for_window_ (window cache size: " << window_cache_.size() << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }

            // Check main cache
            bool is_local_cached = main_cache_ptr_->exists(key);
            if (is_local_cached)
            {
                std::ostringstream oss;
                oss << "key " << key.getKeyDebugstr() << " already exists in main_cache_ for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }

            // Insert into the head of window cache (note from-window-to-main eviction is triggered outside WTinyLFU by EdgeWrapper when total cache is full)
            WTinylfuItem<Key, Value> tmp_item(key, value);
            increaseWindowSize_(tmp_item.getSizeForCapacity());
            window_cache_.push_front(tmp_item);

            // Update key lookup map for window cache
            increaseWindowSize_(key.getKeyLength() + sizeof(windowlist_iterator_t));
            key_lookup_for_window_.insert(std::make_pair(key, window_cache_.begin()));

            return;
        }

        void evictNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims)
        {
            bool evicted = false;
            while (!evicted) // NOTE: evict until window cache is empty or a window victim is evicted
            {
                if (window_cache_.size() > 0) // Window cache is NOT empty
                {
                    // Get window cache victim
                    windowlist_const_iterator_t window_list_const_iter = window_cache_.end();
                    window_list_const_iter--;
                    assert(window_list_const_iter != window_cache_.end());
                    const Key tmp_window_victim_key = window_list_const_iter->getKey();
                    const Value tmp_window_victim_value = window_list_const_iter->getValue();

                    /** only when main_cache is full, evict an obj from the main_cache **/

                    // if main_cache has enough space, insert the obj into main_cache
                    if (main_cache_ptr_->getSizeForCapacity() + tmp_window_victim_key.getKeyLength() + tmp_window_victim_value.getValuesize() + sizeof(int) <= main_cache_max_bytes_) // NOTE: sizeof(int) is metadata of lru_id in SLRU
                    {
                        // NOTE: NOT update victims for window victim due to just moving it from window cache into main cache

                        // Admit window victim into main cache
                        main_cache_ptr_->admit(tmp_window_victim_key, tmp_window_victim_value); // NOTE: main cache will update its cache size usage by itself internally

                        // Remove window victim from window cache
                        decreaseWindowSize_(window_list_const_iter->getSizeForCapacity());
                        window_cache_.erase(window_list_const_iter);
                        
                        // Remove window victim from key lookup map for window cache
                        decreaseWindowSize_(tmp_window_victim_key.getKeyLength() + sizeof(windowlist_iterator_t));
                        lookupmap_iterator_t lookup_map_iter = key_lookup_for_window_.find(tmp_window_victim_key);
                        assert(lookup_map_iter != key_lookup_for_window_.end());
                        key_lookup_for_window_.erase(lookup_map_iter);
                    } // End of non-full main cache
                    else
                    {
                        // compare the frequency of window_victim and main_cache_victim
                        Key tmp_main_victim_key;
                        bool tmp_main_has_victim_key = main_cache_ptr_->getVictimKey(tmp_main_victim_key);
                        assert(tmp_main_has_victim_key);

                        // if window_victim is more frequent, insert it into main_cache
                        if (cbf_->estimate(tmp_window_victim_key) > cbf_->estimate(tmp_main_victim_key))
                        {
                            // NOTE: we will update victims for main victim, yet not for window victim that will be inserted into main cache

                            // Remove main victim from main cache and add into victims
                            Value tmp_main_victim_value;
                            bool tmp_main_is_evict = main_cache_ptr_->evictWithGivenKey(tmp_main_victim_key, tmp_main_victim_value); // NOTE: main cache will update its cache size usage by itself internally
                            assert(tmp_main_is_evict);
                            if (victims.find(tmp_main_victim_key) == victims.end())
                            {
                                victims.insert(std::make_pair(tmp_main_victim_key, tmp_main_victim_value));
                            }

                            // Admit window victim into main cache
                            main_cache_ptr_->admit(tmp_window_victim_key, tmp_window_victim_value); // NOTE: main cache will update its cache size usage by itself internally

                            // Remove window victim from window cache
                            decreaseWindowSize_(window_list_const_iter->getSizeForCapacity());
                            window_cache_.erase(window_list_const_iter);

                            // Remove window victim from key lookup map
                            decreaseWindowSize_(tmp_window_victim_key.getKeyLength() + sizeof(windowlist_iterator_t));
                            lookupmap_iterator_t lookup_map_iter = key_lookup_for_window_.find(tmp_window_victim_key);
                            assert(lookup_map_iter != key_lookup_for_window_.end());
                            key_lookup_for_window_.erase(lookup_map_iter);
                        } // End of window victim is more frequent than main victim
                        else
                        {
                            // NOTE: we will update victims for window victim, yet not for main victim that will still be cached by main cache

                            // Remove window victim from window cache and add into victims
                            decreaseWindowSize_(window_list_const_iter->getSizeForCapacity());
                            window_cache_.erase(window_list_const_iter);
                            if (victims.find(tmp_window_victim_key) == victims.end())
                            {
                                victims.insert(std::make_pair(tmp_window_victim_key, tmp_window_victim_value));
                            }

                            // Remove window victim from key lookup map
                            decreaseWindowSize_(tmp_window_victim_key.getKeyLength() + sizeof(windowlist_iterator_t));
                            lookupmap_iterator_t lookup_map_iter = key_lookup_for_window_.find(tmp_window_victim_key);
                            assert(lookup_map_iter != key_lookup_for_window_.end());
                            key_lookup_for_window_.erase(lookup_map_iter);

                            evicted = true; // Terminate while loop due to evicting a window victim
                        }
                    } // End of full main cache

                    cbf_->update(tmp_window_victim_key);
                } // End of if (window_cache_.size() > 0)
                else
                {
                    assert(window_cache_.size() == 0);

                    // Get main victim from main cache
                    Key tmp_main_victim_key;
                    bool tmp_main_has_victim_key = main_cache_ptr_->getVictimKey(tmp_main_victim_key);
                    assert(tmp_main_has_victim_key);

                    // Remove main victim from main cache and add into victims
                    Value tmp_main_victim_value;
                    bool tmp_main_is_evict = main_cache_ptr_->evictWithGivenKey(tmp_main_victim_key, tmp_main_victim_value); // NOTE: main cache will update its cache size usage by itself internally
                    assert(tmp_main_is_evict);
                    if (victims.find(tmp_main_victim_key) == victims.end())
                    {
                        victims.insert(std::make_pair(tmp_main_victim_key, tmp_main_victim_value));
                    }

                    return;
                } // End of if (window_cache_.size() == 0)
            } // End of while (!evicted)
            return;
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

        uint64_t cbf_max_bytes_;
        uint64_t window_cache_max_bytes_;
        uint64_t main_cache_max_bytes_;

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
            windowlist_iterator_t window_list_iter = lookup_map_iter->second;
            assert(window_list_iter != window_cache_.end());
            fetched_value_in_window = window_list_iter->getValue();

            // Move to the head of window cache
            // -> Remove original item from window cache
            decreaseWindowSize_(window_list_iter->getSizeForCapacity());
            window_cache_.erase(window_list_iter);
            // -> Insert new item into window cache
            Value tmp_value = is_update ? new_value : fetched_value_in_window;
            WTinylfuItem<Key, Value> tmp_item(key, tmp_value);
            increaseWindowSize_(tmp_item.getSizeForCapacity());
            window_cache_.push_front(tmp_item);

            // Update key lookup map for window cache (NO need to update cache size due to no change)
            lookup_map_iter->second = window_cache_.begin();
        }

        // Access main cache
        Value fetched_value_in_main;
        bool is_local_cached_in_main = main_cache_ptr_->get(key, fetched_value_in_main); // Get value
        if (is_local_cached_in_main && is_update)
        {
            // Update value if necessary
            bool tmp_is_local_cached_in_main = main_cache_ptr_->update(key, new_value); // NOTE: SlruLocalCache will update its cache size usage by itself internally
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
            request_bytes_for_main_ += key.getKeyLength() + tmp_value_bytes;

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
