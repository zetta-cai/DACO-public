/*
 * S3fifoCachePolicy: refer to Algorithm 1 in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/S3FIFO.c, yet reimplemented in C++ to escape the dependency on libcachesim.
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
        typedef typename std::unordered_map<Key, s3fifo_iterator, KeyHasher>::iterator map_iterator_t;
        typedef typename std::unordered_map<Key, s3fifo_iterator, KeyHasher>::const_iterator map_const_iterator_t;

        S3fifoCachePolicy()
        {
            s3fifo_small_queue_.clear();
            s3fifo_main_queue_.clear();
            s3fifo_ghost_queue_.clear();
            key_lookup_for_small_.clear();
            key_lookup_for_main_.clear();
            key_lookup_for_ghost_.clear();
            size_ = 0;
        }

        ~S3fifoCachePolicy()
        {
        }

        bool exists(const Key& key) const
        {
            bool is_exist = (key_lookup_for_small_.find(key) != key_lookup_for_small_.end());
            if (!is_exist)
            {
                is_exist = (key_lookup_for_main_.find(key) != key_lookup_for_main_.end());
            }

            return is_exist;
        }

        bool get(const Key& key, Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = key_lookup_for_small_.find(key);
            if (map_iter == key_lookup_for_small_.end()) // NOT found in small cache
            {
                map_iter = key_lookup_for_main_.find(key);
                if (map_iter != key_lookup_for_main_.end()) // Found in main cache
                {
                    is_local_cached = true;
                }
            }
            else // Found in small cache
            {
                is_local_cached = true;
            }

            if (is_local_cached)
            {
                // Update frequency
                s3fifo_iterator list_iter = map_iter->second; // Point to small/main cache
                assert(list_iter != s3fifo_small_queue_.end() || list_iter != s3fifo_main_queue_.end());
                assert(list_iter->getKey()== key);
                list_iter->incrFreq();

                // Get value
                value = list_iter->getValue();
            }

            return is_local_cached;
        }

        bool update(const Key& key, const Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = key_lookup_for_small_.find(key);
            if (map_iter == key_lookup_for_small_.end()) // NOT found in small cache
            {
                map_iter = key_lookup_for_main_.find(key);
                if (map_iter != key_lookup_for_main_.end()) // Found in main cache
                {
                    is_local_cached = true;
                }
            }
            else // Found in small cache
            {
                is_local_cached = true;
            }
            
            if (is_local_cached) // Previous value exists
            {
                // Update frequency
                s3fifo_iterator list_iter = map_iter->second; // Point to small/main cache
                assert(list_iter != s3fifo_small_queue_.end() || list_iter != s3fifo_main_queue_.end());
                assert(list_iter->getKey()== key);
                list_iter->incrFreq();

                // Get previous value
                uint32_t prev_valuesize = list_iter->getValue().getValuesize();
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(prev_valuesize));

                // Update value
                list_iter->setValue(value);
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(value.getValuesize()));
            }

            return is_local_cached;
        }

        void admit(const Key& key, const Value& value)
        {
            // TODO: END HERE
            map_iterator_t map_iter = key_lookup_.find(key);
            if (map_iter != key_lookup_.end()) // Previous list and map entry exist
            {
                std::ostringstream oss;
                oss << "key " << key.getKeystr() << " already exists in key_lookup_ (list size: " << sieve_queue_.size() << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                // Insert a new sieven item to the head of the list
                SieveItem tmp_item(key, value);
                sieve_queue_.emplace_front(tmp_item);
                size_ = Util::uint64Add(size_, tmp_item.getSizeForCapacity());

                // Insert new list iterator for key indexing
                map_iter = key_lookup_.insert(std::pair(key, sieve_queue_.begin())).first;
                assert(map_iter != key_lookup_.end());
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(sieve_iterator)));
            }

            return;
        }

        bool getVictimKey(Key& key)
        {
            bool has_victim_key = false;

            if (sieve_queue_.size() > 0)
            {
                sieve_iterator cur_iter = hand_iter_; // Start from hand pointer
                if (cur_iter == sieve_queue_.end())
                {
                    cur_iter--; // Move the the tail of sieve queue
                }

                while (cur_iter->isVisited())
                {
                    cur_iter->resetVisited();
                    if (cur_iter == sieve_queue_.begin()) // Arrive at the head of sieve queue
                    {
                        cur_iter = sieve_queue_.end(); // Go back to the tail of sieve queue
                    }
                    cur_iter--; // Move to the previous sieve item
                }

                if (cur_iter != sieve_queue_.begin())
                {
                    hand_iter_ = cur_iter;
                    hand_iter_--; // Move to the previous sieve item vs. the current victim
                }
                else
                {
                    hand_iter_ = sieve_queue_.end(); // Will start from the tail of sieve queue in the next eviction
                }

                // Get the tail of the list
                key = cur_iter->getKey();

                has_victim_key = true;
            }

            return has_victim_key;
        }

        bool evictWithGivenKey(const Key& key, Value& value)
        {
            bool is_evict = false;
            
            // Get victim value
            map_iterator_t victim_map_iter = key_lookup_.find(key);
            if (victim_map_iter != key_lookup_.end()) // Key exists
            {
                sieve_iterator victim_list_iter = victim_map_iter->second;
                assert(victim_list_iter != sieve_queue_.end());
                assert(victim_list_iter->getKey() == key);
                value = victim_list_iter->getValue();

                // Remove the corresponding map entry
                key_lookup_.erase(victim_map_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(sieve_iterator)));

                // Remove the corresponding list entry
                const uint64_t victim_size = victim_list_iter->getSizeForCapacity();
                sieve_queue_.erase(victim_list_iter);
                size_ = Util::uint64Minus(size_, victim_size);

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

        std::list<S3fifoItem<Key, Value>> s3fifo_small_queue_; // Small s3fifo items in FIFO order
        std::list<S3fifoItem<Key, Value>> s3fifo_main_queue_; // Main s3fifo items in FIFO order
        std::list<Key> s3fifo_ghost_queue_; // Ghost keys in FIFO order
        std::unordered_map<Key, s3fifo_iterator, KeyHasher> key_lookup_for_small_; // Key indexing for quick lookup in small cache
        std::unordered_map<Key, s3fifo_iterator, KeyHasher> key_lookup_for_main_; // Key indexing for quick lookup in main cache
        std::unordered_set<Key, std::list<Key>::iterator, KeyHasher> key_lookup_for_ghost_; // Key indexing for quick lookup in ghost cache

        uint64_t size_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string S3fifoCachePolicy<Key, Value, KeyHasher>::kClassName("S3fifoCachePolicy");

} // namespace covered

#endif