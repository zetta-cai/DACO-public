/*
 * S3fifoCachePolicy: refer to Algorithm 1 in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/S3FIFO.c, yet directly reimplement in C++ due to simplicity of S3FIFO to escape the dependency on libcachesim and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + insufficient cache size usage calculation).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: NO need to use optimized data structures, as system bottleneck is network propagation latency in geo-distributed edge settings.
 * 
 * NOTE: S3-FIFO is a coarse-grained cache replacement policy, which cannot find and evict a given object; also mentioned in lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/S3FIFO.c, which does not implement S3FIFO_to_evict() -> it just evicts non-specified objects from small/main FIFO caches.
 * 
 * By Siyuan Sheng (2024.01.03).
 */

#ifndef S3FIFO_CACHE_POLICY_H
#define S3FIFO_CACHE_POLICY_H

#include <list>
#include <unordered_map>

#include "common/util.h"

#define S3FIFO_SMALL_CACHE_RATIO 0.1

namespace covered
{
    template <typename Key, typename Value>
    class S3fifoItem
    {
    public:
        S3fifoItem() : key_(), value_(), freq_(0)
        {
        }

        S3fifoItem(const Key& key, const Value& value)
        {
            key_ = key;
            value_ = value;
            freq_ = 0;
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

        uint8_t getFreq() const
        {
            return freq_;
        }

        void incrFreq()
        {
            if (freq_ < 3) // Capped to 3
            {
                freq_++;
            }
            return;
        }

        void decrFreq()
        {
            assert(freq_ > 0);
            freq_--;
            return;
        }

        const double getSizeForCapacity() const
        {
            // NOTE: count freq_ as 1B/4.0 = 2b
            return static_cast<double>(key_.getKeyLength()) + static_cast<double>(value_.getValuesize()) + static_cast<double>(sizeof(uint8_t))/4.0;
        }

        const S3fifoItem& operator=(const S3fifoItem& other)
        {
            if (this != &other)
            {
                key_ = other.key_;
                value_ = other.value_;
                freq_ = other.freq_;
            }
            return *this;
        }
    private:
        Key key_;
        Value value_;
        uint8_t freq_; // Frequency capped to 3 (count 2 bits for cache size usage)
    };

    template <typename Key, typename Value, typename KeyHasher>
    class S3fifoCachePolicy
    {
    public:
        using s3fifo_iterator = typename std::list<S3fifoItem<Key, Value>>::iterator;
        using s3fifo_const_iterator = typename std::list<S3fifoItem<Key, Value>>::const_iterator;
        using s3fifo_ghost_iterator = typename std::list<Key>::iterator;
        typedef typename std::unordered_map<Key, s3fifo_iterator, KeyHasher>::iterator map_iterator_t;
        typedef typename std::unordered_map<Key, s3fifo_iterator, KeyHasher>::const_iterator map_const_iterator_t;
        typedef typename std::unordered_map<Key, s3fifo_ghost_iterator, KeyHasher>::iterator ghost_map_iterator_t;

        S3fifoCachePolicy(const uint64_t& capacity_bytes) : capacity_bytes_(capacity_bytes)
        {
            s3fifo_small_queue_.clear();
            s3fifo_main_queue_.clear();
            s3fifo_ghost_queue_.clear();
            key_lookup_for_small_.clear();
            key_lookup_for_main_.clear();
            key_lookup_for_ghost_.clear();

            size_for_small_ = 0;
            size_for_main_ = 0;
            size_for_ghost_ = 0;
        }

        ~S3fifoCachePolicy()
        {
        }

        bool exists(const Key& key)
        {
            bool unused_is_small_cache = false;
            map_iterator_t unused_map_iter;
            bool is_exist = exists_(key, unused_is_small_cache, unused_map_iter);
            UNUSED(unused_is_small_cache);
            UNUSED(unused_map_iter);

            return is_exist;
        }

        bool get(const Key& key, Value& value)
        {
            bool unused_is_small_cache = false;
            map_iterator_t map_iter;
            bool is_local_cached = exists_(key, unused_is_small_cache, map_iter);

            if (is_local_cached)
            {
                // Update frequency
                s3fifo_iterator list_iter = map_iter->second; // Point to small/main cache
                assert(list_iter != s3fifo_small_queue_.end() || list_iter != s3fifo_main_queue_.end());
                assert(list_iter->getKey()== key);
                list_iter->incrFreq();

                // Get value
                value = list_iter->getValue();

                // Remove from ghost cache if any
                removeFromGhost_(key);
            }

            return is_local_cached;
        }

        bool update(const Key& key, const Value& value)
        {
            bool is_small_cache = false;
            map_iterator_t map_iter;
            bool is_local_cached = exists_(key, is_small_cache, map_iter);

            if (is_local_cached) // Previous value exists
            {
                // Update frequency
                s3fifo_iterator list_iter = map_iter->second; // Point to small/main cache
                assert(list_iter != s3fifo_small_queue_.end() || list_iter != s3fifo_main_queue_.end());
                assert(list_iter->getKey()== key);
                list_iter->incrFreq();

                // Get previous value
                uint32_t prev_valuesize = list_iter->getValue().getValuesize();
                if (is_small_cache)
                {
                    size_for_small_ -= static_cast<double>(prev_valuesize);
                }
                else
                {
                    size_for_main_ -= static_cast<double>(prev_valuesize);
                }

                // Update value
                list_iter->setValue(value);
                if (is_small_cache)
                {
                    size_for_small_ += static_cast<double>(value.getValuesize());
                }
                else
                {
                    size_for_main_ += static_cast<double>(value.getValuesize());
                }

                // Remove from ghost cache if any
                removeFromGhost_(key);

                // Fix double precision issue if any
                tryToFixDoublePrecisionIssue_();
            }

            return is_local_cached;
        }

        bool canAdmit(const uint32_t& objsize) const
        {
            return objsize <= (S3FIFO_SMALL_CACHE_RATIO * capacity_bytes_); // Need <= small cache size
        }

        void admit(const Key& key, const Value& value)
        {
            bool is_small_cache = false;
            map_iterator_t map_iter;
            bool is_local_cached = exists_(key, is_small_cache, map_iter);

            if (is_local_cached) // Previous list and map entry exist
            {
                std::string lookup_table_name = "";
                uint32_t queue_size = 0;
                if (is_small_cache)
                {
                    lookup_table_name = "key_lookup_for_small_";
                    queue_size = s3fifo_small_queue_.size();
                }
                else
                {
                    lookup_table_name = "key_lookup_for_main_";
                    queue_size = s3fifo_main_queue_.size();
                }

                std::ostringstream oss;
                oss << "key " << key.getKeyDebugstr() << " already exists in " << lookup_table_name << " (list size: " << queue_size << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                // Check ghost cache
                bool hit_on_ghost = (key_lookup_for_ghost_.find(key) != key_lookup_for_ghost_.end());

                // Insert a new s3fifo item with map entry
                S3fifoItem<Key, Value> tmp_item(key, value);
                if (hit_on_ghost) // Insert into main cache
                {
                    insertIntoMain_(tmp_item);
                }
                else // Insert into small cache
                {
                    insertIntoSmall_(tmp_item);
                }

                // Remove from ghost cache if any
                removeFromGhost_(key);
            }

            return;
        }

        void evictNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims)
        {
            if (size_for_small_ >= S3FIFO_SMALL_CACHE_RATIO * static_cast<double>(capacity_bytes_)) // Evict from small cache
            {
                evictFromSmall_(victims);
            }
            else // Evict from main cache
            {
                evictFromMain_(victims);
            }

            return;
        }

        uint64_t getSizeForCapacity() const
        {
            const double total_size = size_for_small_ + size_for_main_ + size_for_ghost_;
            return static_cast<uint64_t>(total_size);
        }

    private:

        bool exists_(const Key& key, bool& is_small_cache, map_iterator_t& map_iter)
        {
            bool is_local_cached = false;
            is_small_cache = false;

            map_iter = key_lookup_for_small_.find(key);
            if (map_iter == key_lookup_for_small_.end()) // NOT found in small cache
            {
                map_iter = key_lookup_for_main_.find(key);
                if (map_iter != key_lookup_for_main_.end()) // Found in main cache
                {
                    is_small_cache = false;
                    is_local_cached = true;
                }
            }
            else // Found in small cache
            {
                is_small_cache = true;
                is_local_cached = true;
            }

            return is_local_cached;
        }

        void checkGhostCacheSize_()
        {
            // NOTE: ghost cache have at most the same number of keys as in main cache
            while (s3fifo_ghost_queue_.size() > s3fifo_main_queue_.size())
            {
                evictFromGhost_();
            }
            return;
        }

        void evictFromSmall_(std::unordered_map<Key, Value, KeyHasher>& victims)
        {
            bool has_evicted = false;
            while (!has_evicted && s3fifo_small_queue_.size() > 0) // Evict until one object is evicted from small cache or small cache becomes empty
            {
                // Start from the tail of the small cache
                s3fifo_iterator victim_list_iter = s3fifo_small_queue_.end();
                victim_list_iter--;
                const Key& victim_key = victim_list_iter->getKey();

                if (victim_list_iter->getFreq() > 1) // Move to main cache
                {
                    // Insert the victim object into main cache with map entry
                    insertIntoMain_(*victim_list_iter); // Will increase size_for_main_

                    if (size_for_main_ >= 0.9 * static_cast<double>(capacity_bytes_)) // Main cache is full
                    {
                        evictFromMain_(victims);
                    }
                }
                else // Evict to ghost cache
                {
                    // Insert the victim key into ghost cache with map entry
                    insertIntoGhost_(victim_key);

                    // Add into victims
                    victims.insert(std::pair(victim_key, victim_list_iter->getValue()));

                    // Will break the while loop
                    has_evicted = true;
                }

                // Remove the corresponding map entry
                key_lookup_for_small_.erase(victim_key);
                size_for_small_ -= static_cast<double>(victim_key.getKeyLength() + sizeof(s3fifo_iterator));

                // Remove the victim object from small cache
                const double victim_size = victim_list_iter->getSizeForCapacity();
                s3fifo_small_queue_.erase(victim_list_iter);
                size_for_small_ -= victim_size;
            }

            // Fix double precision issue if any
            tryToFixDoublePrecisionIssue_();

            return;
        }

        void evictFromMain_(std::unordered_map<Key, Value, KeyHasher>& victims)
        {
            bool has_evicted = false;
            while (!has_evicted && s3fifo_main_queue_.size() > 0) // Evict until one object is evicted from main cache or main cache becomes empty
            {
                // Start from the tail of the main cache
                s3fifo_iterator victim_list_iter = s3fifo_main_queue_.end();
                victim_list_iter--;
                const Key& victim_key = victim_list_iter->getKey();

                if (victim_list_iter->getFreq() > 0) // FIFO reinsertion
                {
                    // Copy before remove
                    S3fifoItem<Key, Value> tmp_item = *victim_list_iter;
                    s3fifo_main_queue_.erase(victim_list_iter);
                    victim_list_iter = s3fifo_main_queue_.end();

                    // Reinsert into the head of the main cache
                    s3fifo_main_queue_.emplace_front(tmp_item);
                    victim_list_iter = s3fifo_main_queue_.begin();
                    victim_list_iter->decrFreq();

                    // Update the corresponding map entry
                    map_iterator_t map_iter = key_lookup_for_main_.find(victim_key);
                    assert(map_iter != key_lookup_for_main_.end());
                    map_iter->second = victim_list_iter;

                    // NOTE: NO need to update size_for_main_ during FIFO reinsertion
                }
                else // Remove from main cache
                {
                    // Add into victims
                    victims.insert(std::pair(victim_key, victim_list_iter->getValue()));

                    // Remove the correpsonding map entry
                    key_lookup_for_main_.erase(victim_key);
                    size_for_main_ -= static_cast<double>(victim_key.getKeyLength() + sizeof(s3fifo_iterator));

                    // Remove the victim object from main cache
                    const double victim_size = victim_list_iter->getSizeForCapacity();
                    s3fifo_main_queue_.erase(victim_list_iter);
                    size_for_main_ -= victim_size;

                    // Will break the while loop
                    has_evicted = true;
                }
            }

            // Limit the size of ghost cache as mentioned in original paper
            checkGhostCacheSize_();

            // Fix double precision issue if any
            tryToFixDoublePrecisionIssue_();

            return;
        }

        void evictFromGhost_()
        {
            assert(s3fifo_ghost_queue_.size() > 0);

            // Get the tail of ghost cache
            s3fifo_ghost_iterator list_iter = s3fifo_ghost_queue_.end();
            list_iter--;

            // Remove the corresponding map entry
            ghost_map_iterator_t map_iter = key_lookup_for_ghost_.find(*list_iter);
            assert(map_iter != key_lookup_for_ghost_.end());
            key_lookup_for_ghost_.erase(map_iter);
            size_for_ghost_ -= static_cast<double>(list_iter->getKeyLength() + sizeof(s3fifo_ghost_iterator));

            // Remove the tail key from ghost cache
            s3fifo_ghost_queue_.erase(list_iter);
            size_for_ghost_ -= static_cast<double>(list_iter->getKeyLength());

            // Fix double precision issue if any
            tryToFixDoublePrecisionIssue_();

            return;
        }

        void removeFromGhost_(const Key& key)
        {
            // Remove from ghost cache if any
            ghost_map_iterator_t map_iter = key_lookup_for_ghost_.find(key);
            if (map_iter != key_lookup_for_ghost_.end())
            {
                // Remove the victim key from ghost cache
                s3fifo_ghost_iterator list_iter = map_iter->second;
                assert(list_iter != s3fifo_ghost_queue_.end());
                assert(*list_iter == key);
                s3fifo_ghost_queue_.erase(list_iter);
                size_for_ghost_ -= static_cast<double>(key.getKeyLength());

                // Remove the corresponding map entry
                key_lookup_for_ghost_.erase(map_iter);
                size_for_ghost_ -= static_cast<double>(key.getKeyLength() + sizeof(s3fifo_ghost_iterator));
            }

            // Fix double precision issue if any
            tryToFixDoublePrecisionIssue_();

            return;
        }

        void insertIntoSmall_(const S3fifoItem<Key, Value>& item)
        {
            const Key& key = item.getKey();

            // Insert a new s3fifo item to the head of the small cache list
            s3fifo_small_queue_.emplace_front(item);
            size_for_small_ += item.getSizeForCapacity();

            // Insert new list iterator for key indexing
            map_iterator_t map_iter = key_lookup_for_small_.insert(std::pair(key, s3fifo_small_queue_.begin())).first;
            assert(map_iter != key_lookup_for_small_.end());
            size_for_small_ += static_cast<double>(key.getKeyLength() + sizeof(s3fifo_iterator));

            return;
        }

        void insertIntoMain_(const S3fifoItem<Key, Value>& item)
        {
            const Key& key = item.getKey();

            // Insert a new s3fifo item to the head of the main cache list
            s3fifo_main_queue_.emplace_front(item);
            size_for_main_ += item.getSizeForCapacity();

            // Insert new list iterator for key indexing
            map_iterator_t map_iter = key_lookup_for_main_.insert(std::pair(key, s3fifo_main_queue_.begin())).first;
            assert(map_iter != key_lookup_for_main_.end());
            size_for_main_ += static_cast<double>(key.getKeyLength() + sizeof(s3fifo_iterator));

            return;
        }

        void insertIntoGhost_(const Key& victim_key)
        {
            assert(key_lookup_for_ghost_.find(victim_key) == key_lookup_for_ghost_.end());

            // Insert the victim key into ghost cache
            s3fifo_ghost_queue_.emplace_front(victim_key);
            size_for_ghost_ += static_cast<double>(victim_key.getKeyLength());

            // Insert corresponding map entry
            ghost_map_iterator_t map_iter = key_lookup_for_ghost_.insert(std::pair(victim_key, s3fifo_ghost_queue_.begin())).first;
            assert(map_iter != key_lookup_for_ghost_.end());
            size_for_ghost_ += static_cast<double>(victim_key.getKeyLength() + sizeof(s3fifo_ghost_iterator));

            // Limit the size of ghost cache as mentioned in original paper
            checkGhostCacheSize_();

            return;
        }

        void tryToFixDoublePrecisionIssue_()
        {
            if (size_for_small_ < 0)
            {
                size_for_small_ = 0;
            }
            if (size_for_main_ < 0)
            {
                size_for_main_ = 0;
            }
            if (size_for_ghost_ < 0)
            {
                size_for_ghost_ = 0;
            }
            return;
        }

    private:
        static const std::string kClassName;

        const uint64_t capacity_bytes_; // Cache capacity (used for eviction)

        std::list<S3fifoItem<Key, Value>> s3fifo_small_queue_; // Small s3fifo items in FIFO order
        std::list<S3fifoItem<Key, Value>> s3fifo_main_queue_; // Main s3fifo items in FIFO order
        std::list<Key> s3fifo_ghost_queue_; // Ghost keys in FIFO order
        std::unordered_map<Key, s3fifo_iterator, KeyHasher> key_lookup_for_small_; // Key indexing for quick lookup in small cache
        std::unordered_map<Key, s3fifo_iterator, KeyHasher> key_lookup_for_main_; // Key indexing for quick lookup in main cache
        std::unordered_map<Key, s3fifo_ghost_iterator, KeyHasher> key_lookup_for_ghost_; // Key indexing for quick lookup in ghost cache

        double size_for_small_; // Cache size usage of small cache in units of bytes (used for eviction and capacity limitation)
        double size_for_main_; // Cache size usage of main cache in units of bytes (used for eviction and capacity limitation)
        double size_for_ghost_; // Cache size usage of ghost cache in units of bytes (used for capacity limitation)
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string S3fifoCachePolicy<Key, Value, KeyHasher>::kClassName("S3fifoCachePolicy");

} // namespace covered

#endif