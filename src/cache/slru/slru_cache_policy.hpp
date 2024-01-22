/*
 * SlruCachePolicy: refer to Algorithm Fig.3 in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/SLRU.c, yet directly reimplement in C++ due to simplicity of SLRU to escape the dependency on libcachesim and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + insufficient cache size usage calculation).
 *
 * NOTE: SLRU is a fine-grained replacement policy, which uses multiple LRU lists to lookup with SLRU cool operations and admit/evict starting from the lowest LRU list (used by TinyLFU for W-TinfyLFU; see src/cache/tinylfu/tinylfu_cache_policy.hpp).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: NO need to use optimized data structures, as system bottleneck is network propagation latency in geo-distributed edge settings.
 * 
 * By Siyuan Sheng (2024.01.22).
 */

#ifndef SLRU_CACHE_POLICY_HPP
#define SLRU_CACHE_POLICY_HPP

#include <list>
#include <unordered_map>
#include <vector>

#include "common/util.h"

namespace covered
{
    template <typename Key, typename Value>
    class SlruItem
    {
    public:
        SlruItem(): key_(), value_(), lru_id_(0)
        {
        }

        SlruItem(const Key& key, const Value& value, const int& lru_id)
        {
            key_ = key;
            value_ = value;
            lru_id_ = lru_id;
        }

        Key getKey() const
        {
            return key_;
        }

        Value getValue() const
        {
            return value_;
        }

        int getLruId() const
        {
            return lru_id_;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(key_.getKeyLength() + value_.getValuesize() + sizeof(int));
        }

        const SlruItem& operator=(const SlruItem& other)
        {
            if (this != &other)
            {
                key_ = other.key_;
                value_ = other.value_;
                lru_id_ = other.lru_id_;
            }
            return *this;
        }
    private:
        Key key_;
        Value value_;
        int lru_id_; // In which LRU list
    };

    template <typename Key, typename Value>
    class SlruKeyLookupIter
    {
    public:
        typedef typename std::list<SlruItem<Key, Value>>::iterator lrulist_iterator_t;

        SlruKeyLookupIter(const lrulist_iterator_t& lru_list_iter) : lru_id_(lru_list_iter->getLruId())
        {
            lru_list_iter_ =  lru_list_iter;
        }

        int getLruId() const
        {
            return lru_id_;
        }

        const lrulist_iterator_t& getLruListIterConstRef() const
        {
            return lru_list_iter_;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(sizeof(lrulist_iterator_t)); // Siyuan: ONLY count cache size usage of one iterator (we maintain a boolean and two iterators just for impl trick)
        }

        const SlruKeyLookupIter<Key, Value>& operator=(const SlruKeyLookupIter<Key, Value>& other)
        {
            if (this != &other)
            {
                lru_id_ = other.lru_id_;
                lru_list_iter_ = other.lru_list_iter_;
            }
            return *this;
        }
    private:
        int lru_id_;
        lrulist_iterator_t lru_list_iter_;
    };

    template <typename Key, typename Value, typename KeyHasher>
    class SlruCachePolicy
    {
    public:
        typedef typename std::list<SlruItem<Key, Value>>::iterator lrulist_iterator_t;
        typedef typename std::list<SlruItem<Key, Value>>::const_iterator lrulist_const_iterator_t;
        typedef typename std::unordered_map<Key, SlruKeyLookupIter<Key, Value>, KeyHasher>::iterator lookupmap_iterator_t;
        typedef typename std::unordered_map<Key, SlruKeyLookupIter<Key, Value>, KeyHasher>::const_iterator lookupmap_const_iterator_t;
    private:
        // Function declarations
        // Cache eviction
        bool access_(const Key& key, Value& fetched_value, const bool& is_update = false, const Value& new_value = Value());
        void cool_(const int& lru_id);
        // Cache size calculation
        void decreaseLruAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id);
        void increaseLruAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id);
        void decreaseTotalSize_(const uint64_t& decrease_bytes);
        void increaseTotalSize_(const uint64_t& increase_bytes);
    public:
        SlruCachePolicy(const uint64_t& capacity_bytes) : capacity_bytes_(capacity_bytes)
        {
            // Use 4 LRU lists equally partitioning cache capacity bytes by default (refer to SLRU_init() in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/SLRU.c)
            n_seg_ = 4;
            lru_lists_.resize(n_seg_, std::list<SlruItem<Key, Value>>());
            lru_n_bytes_.resize(n_seg_, 0);
            lru_max_n_bytes_.resize(n_seg_, capacity_bytes / n_seg_);

            key_lookup_.clear();
            size_ = 0;
        }

        ~SlruCachePolicy()
        {
            lru_lists_.clear();
            lru_n_bytes_.clear();
            lru_max_n_bytes_.clear();
            
            key_lookup_.clear();
        }

        bool exists(const Key& key) const
        {
            bool is_local_cached = false;

            lookupmap_const_iterator_t lookup_map_const_iter = key_lookup_.find(key);
            if (lookup_map_const_iter != key_lookup_.end()) // Key exists in some LRU list
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
                std::ostringstream oss;
                oss << "key " << key.getKeystr() << " already exists in key_lookup_ (";
                for (int i = 0; i < n_seg_; i++)
                {
                    oss << "lru_lists_[" << i << "].size() = " << lru_lists_[i].size();
                    if (i != n_seg_ - 1)
                    {
                        oss << ", ";
                    }
                }
                oss << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                // Find the lowest LRU with space for insertion
                uint64_t incoming_bytes = SlruItem<Key, Value>(key, value, 0).getSizeForCapacity(); // NOTE: just use 0 for metadata size usage, instead of the real lru id
                int nth_seg = -1;
                for (int i = 0; i < n_seg_; i++) {
                    if (lru_n_bytes_[i] + incoming_bytes <= lru_max_n_bytes_[i]) {
                        nth_seg = i;
                        break;
                    }
                }

                if (nth_seg == -1) {
                    // Siyuan: disable internal cache eviction, which will be triggered outside SLRU by EdgeWrapper
                    // // No space for insertion
                    //     while (cache->occupied_byte + req->obj_size + cache->obj_md_size >
                    //         cache->cache_size) {
                    //     cache->evict(cache, req);
                    // }
                    nth_seg = 0;
                }

                // Insert into the lowest LRU with space
                assert(nth_seg < n_seg_);
                SlruItem<Key, Value> tmp_item(key, value, nth_seg);
                std::list<SlruItem<Key, Value>>& tmp_lru_list_ref = lru_lists_[nth_seg];
                increaseLruAndTotalSize_(tmp_item.getSizeForCapacity(), nth_seg);
                tmp_lru_list_ref.push_front(tmp_item);

                // Update key lookup map
                SlruKeyLookupIter<Key, Value> tmp_lookup_iter(tmp_lru_list_ref.begin());
                increaseTotalSize_(key.getKeyLength() + tmp_lookup_iter.getSizeForCapacity());
                key_lookup_.insert(std::make_pair(key, tmp_lookup_iter));
            }

            return;
        }

        bool getVictimKey(Key& key)
        {
            bool has_victim_key = false;

            for (int i = 0; i < n_seg_; i++) {
                if (lru_n_bytes_[i] > 0) {
                    key = lru_lists_[i].back().getKey();
                    has_victim_key = true;
                }
            }

            return has_victim_key;
        }

        bool evictWithGivenKey(const Key& key, Value& value)
        {
            bool is_evict = false;
            
            lookupmap_iterator_t lookup_map_iter = key_lookup_.find(key);
            if (lookup_map_iter != key_lookup_.end()) {
                int lru_id = lookup_map_iter->second.getLruId();
                const lrulist_iterator_t& lru_list_iter_const_ref = lookup_map_iter->second.getLruListIterConstRef();
                assert(lru_list_iter_const_ref != lru_lists_[lru_id].end());
                assert(lru_id == lru_list_iter_const_ref->getLruId());

                // Get value
                value = lru_list_iter_const_ref->getValue();

                // Remove from the corresponding LRU list
                decreaseLruAndTotalSize_(lru_list_iter_const_ref->getSizeForCapacity(), lru_id);
                lru_lists_[lru_id].erase(lru_list_iter_const_ref);

                // Remove from the key lookup map
                decreaseTotalSize_(key.getKeyLength() + lookup_map_iter->second.getSizeForCapacity());
                key_lookup_.erase(lookup_map_iter);

                is_evict = true;
            }

            return is_evict;
        }

        uint64_t getSizeForCapacity() const
        {
            return size_;
        }
    private:
        static const std::string kClassName;

        std::vector<std::list<SlruItem<Key, Value>>> lru_lists_; // lru_lists_[i].size() is lru_n_objs[i]
        std::vector<uint64_t> lru_n_bytes_;
        std::vector<uint64_t> lru_max_n_bytes_;
        int n_seg_;

        std::unordered_map<Key, SlruKeyLookupIter<Key, Value>, KeyHasher> key_lookup_; // Key indexing for quick lookup
        uint64_t size_; // in units of bytes
        const uint64_t capacity_bytes_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string SlruCachePolicy<Key, Value, KeyHasher>::kClassName("SlruCachePolicy");

    // Cache access for get and update

    template <typename Key, typename Value, typename KeyHasher>
    bool SlruCachePolicy<Key, Value, KeyHasher>::access_(const Key& key, Value& fetched_value, const bool& is_update, const Value& new_value)
    {
        bool is_local_cached = false;

        lookupmap_iterator_t lookup_map_iter = key_lookup_.find(key);
        if (lookup_map_iter == key_lookup_.end())
        {
            is_local_cached = false;
        }
        else
        {
            is_local_cached = true;

            // Get value
            int lru_id = lookup_map_iter->second.getLruId();
            assert(lru_id < n_seg_);
            const lrulist_iterator_t& lru_list_iter_const_ref = lookup_map_iter->second.getLruListIterConstRef();
            std::list<SlruItem<Key, Value>>& tmp_lru_list_ref = lru_lists_[lru_id];
            assert(lru_list_iter_const_ref != tmp_lru_list_ref.end());
            assert(lru_list_iter_const_ref->getKey() == key);
            assert(lru_list_iter_const_ref->getLruId() == lru_id);
            fetched_value = lru_list_iter_const_ref->getValue();

            if (lru_id == n_seg_ - 1) { // The last LRU list
                // Move obj to the head of the last LRU list (NO need to update cache size usage due to no changes)
                // -> Remove from the last LRU list
                tmp_lru_list_ref.erase(lru_list_iter_const_ref);
                // -> Insert into the head of the last LRU list
                Value tmp_value = is_update ? new_value : fetched_value;
                SlruItem<Key, Value> tmp_item(key, tmp_value, lru_id);
                tmp_lru_list_ref.push_front(tmp_item);

                // Update key lookup map (NO need to update cache size usage due to no changes)
                SlruKeyLookupIter<Key, Value> tmp_lookup_iter(tmp_lru_list_ref.begin());
                lookup_map_iter->second = tmp_lookup_iter;
            } else { // Not the last LRU list
                // Move obj to the head of the next LRU list
                // -> Remove from the current LRU list
                decreaseLruAndTotalSize_(lru_list_iter_const_ref->getSizeForCapacity(), lru_id);
                tmp_lru_list_ref.erase(lru_list_iter_const_ref);
                // -> Insert into the head of the next LRU list
                Value tmp_value = is_update ? new_value : fetched_value;
                SlruItem<Key, Value> tmp_item(key, tmp_value, lru_id + 1);
                increaseLruAndTotalSize_(tmp_item.getSizeForCapacity(), lru_id + 1);
                lru_lists_[lru_id + 1].push_front(tmp_item);

                // Update key lookup map (NO need to update cache size usage due to no changes)
                SlruKeyLookupIter<Key, Value> tmp_lookup_iter(lru_lists_[lru_id + 1].begin());
                lookup_map_iter->second = tmp_lookup_iter;

                // NOTE: lru_id + 1 >= 1 > 0, which must NOT be the first LRU list
                while (lru_n_bytes_[lru_id + 1] > lru_max_n_bytes_[lru_id + 1]) {
                    // if the next LRU list is full
                    cool_(lru_id + 1);
                }
                //DEBUG_ASSERT(cache->occupied_byte <= cache->cache_size); // Siyuan: NO need, as we will check cache eviction outside SLRU by EdgeWrapper
            }
        }

        return is_local_cached;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void SlruCachePolicy<Key, Value, KeyHasher>::cool_(const int& lru_id)
    {
        assert(lru_id < n_seg_);

        // NOTE: we never invoke cool_() for the first LRU list, as we will evict victim from LRU 0 outside SLRU by EdgeWrapper
        assert(lru_id > 0);
        // if (lru_id == 0)
        // {
        //     // (Siyuan) NOTE: we do NOT want internal eviction, so we leave the cached object here for LRU 0 during cool_()
        //     //return SLRU_evict(cache, req);
        //     return;
        // }

        // Get tail element from the given LRU list
        std::list<SlruItem<Key, Value>>& tmp_lru_list_ref = lru_lists_[lru_id];
        assert(tmp_lru_list_ref.size() > 0);
        lrulist_const_iterator_t tmp_lru_list_const_iter = tmp_lru_list_ref.end();
        tmp_lru_list_const_iter--;
        assert(tmp_lru_list_const_iter->getLruId() == lru_id);

        // Move the tail element from the given LRU list to the previous LRU list
        // -> Remove from the given LRU list
        SlruItem tmp_item(tmp_lru_list_const_iter->getKey(), tmp_lru_list_const_iter->getValue(), lru_id - 1); // Prepare item to be inserted into the previous LRU list
        decreaseLruAndTotalSize_(tmp_lru_list_const_iter->getSizeForCapacity(), lru_id);
        tmp_lru_list_ref.erase(tmp_lru_list_const_iter);
        // -> Insert into the previous LRU list
        increaseLruAndTotalSize_(tmp_item.getSizeForCapacity(), lru_id - 1);
        lru_lists_[lru_id - 1].push_front(tmp_item);
        
        // Update key lookup map (NO need to update cache size usage due to no changes)
        lookupmap_iterator_t lookup_map_iter = key_lookup_.find(tmp_item.getKey());
        assert(lookup_map_iter != key_lookup_.end());
        lookup_map_iter->second = SlruKeyLookupIter<Key, Value>(lru_lists_[lru_id - 1].begin());

        // If lower LRUs are full
        // Siyuan: if lru_id - 1 = 0, we do NOT invoke cool_() recursively, which may incur infinite loop due to disabling internal eviction in the first LRU list
        while ((lru_id - 1 > 0) && (lru_n_bytes_[lru_id - 1] > lru_max_n_bytes_[lru_id - 1])) {
            cool_(lru_id - 1);
        }
    }

    // Cache size usage calculation

    template <typename Key, typename Value, typename KeyHasher>
    void SlruCachePolicy<Key, Value, KeyHasher>::decreaseLruAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id)
    {
        assert(lru_id < n_seg_);

        if (lru_n_bytes_[lru_id] <= decrease_bytes)
        {
            lru_n_bytes_[lru_id] = 0;
        }
        else
        {
            lru_n_bytes_[lru_id] -= decrease_bytes;
        }

        decreaseTotalSize_(decrease_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void SlruCachePolicy<Key, Value, KeyHasher>::increaseLruAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id)
    {
        assert(lru_id < n_seg_);

        lru_n_bytes_[lru_id] += increase_bytes;

        increaseTotalSize_(increase_bytes);
        return;
    }

    template <typename Key, typename Value, typename KeyHasher>
    void SlruCachePolicy<Key, Value, KeyHasher>::decreaseTotalSize_(const uint64_t& decrease_bytes)
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
    void SlruCachePolicy<Key, Value, KeyHasher>::increaseTotalSize_(const uint64_t& increase_bytes)
    {
        size_ += increase_bytes;
        return;
    }

} // namespace covered

#endif // SLRU_CACHE_POLICY_HPP
