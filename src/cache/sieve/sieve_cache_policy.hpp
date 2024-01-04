/*
 * SieveCachePolicy: refer to Algorithm 1 in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/Sieve.c, yet directly reimplement in C++ due to simplicity of SIEVE to escape the dependency on libcachesim and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: NO need to use optimized data structures, as system bottleneck is network propagation latency in geo-distributed edge settings.
 * 
 * By Siyuan Sheng (2024.01.03).
 */

#ifndef SIEVE_CACHE_POLICY_HPP
#define SIEVE_CACHE_POLICY_HPP

#include <list>
#include <unordered_map>

#include "common/util.h"

namespace covered
{
    template <typename Key, typename Value>
    class SieveItem
    {
    public:
        SieveItem() : key_(), value_(), visited_(false)
        {
        }

        SieveItem(const Key& key, const Value& value)
        {
            key_ = key;
            value_ = value;
            visited_ = false;
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

        bool isVisited() const
        {
            return visited_;
        }

        void setVisited()
        {
            visited_ = true;
            return;
        }

        void resetVisited()
        {
            visited_ = false;
            return;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(key_.getKeyLength() + value_.getValuesize() + sizeof(bool));
        }

        const SieveItem& operator=(const SieveItem& other)
        {
            if (this != &other)
            {
                key_ = other.key_;
                value_ = other.value_;
                visited_ = other.visited_;
            }
            return *this;
        }
    private:
        Key key_;
        Value value_;
        bool visited_;
    };

    template <typename Key, typename Value, typename KeyHasher>
    class SieveCachePolicy
    {
    public:
        using sieve_iterator = typename std::list<SieveItem<Key, Value>>::iterator;
        using sieve_const_iterator = typename std::list<SieveItem<Key, Value>>::const_iterator;
        typedef typename std::unordered_map<Key, sieve_iterator, KeyHasher>::iterator map_iterator_t;
        typedef typename std::unordered_map<Key, sieve_iterator, KeyHasher>::const_iterator map_const_iterator_t;

        SieveCachePolicy()
        {
            sieve_queue_.clear();
            key_lookup_.clear();
            hand_iter_ = sieve_queue_.end();
            size_ = 0;
        }

        ~SieveCachePolicy()
        {
        }

        bool exists(const Key& key) const
        {
            return key_lookup_.find(key) != key_lookup_.end();
        }

        bool get(const Key& key, Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = key_lookup_.find(key);
            if (map_iter == key_lookup_.end())
            {
                is_local_cached = false;
            }
            else
            {
                // Update visited flag
                sieve_iterator list_iter = map_iter->second;
                assert(list_iter != sieve_queue_.end());
                assert(list_iter->getKey()== key);
                list_iter->setVisited();

                // Get value
                value = list_iter->getValue();

                is_local_cached = true;
            }

            return is_local_cached;
        }

        bool update(const Key& key, const Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = key_lookup_.find(key);
            if (map_iter != key_lookup_.end()) // Previous value exists
            {
                // Update visited flag
                sieve_iterator list_iter = map_iter->second;
                assert(list_iter != sieve_queue_.end());
                assert(list_iter->getKey() == key);
                list_iter->setVisited();

                // Get previous value
                uint32_t prev_valuesize = list_iter->getValue().getValuesize();
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(prev_valuesize));

                // Update value
                list_iter->setValue(value);
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(value.getValuesize()));

                is_local_cached = true;
            }

            return is_local_cached;
        }

        void admit(const Key& key, const Value& value)
        {
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
                // Insert a new sieve item to the head of the list
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

        std::list<SieveItem<Key, Value>> sieve_queue_; // SIEVE items in CLOCK order
        std::unordered_map<Key, sieve_iterator, KeyHasher> key_lookup_; // Key indexing for quick lookup
        sieve_iterator hand_iter_;

        uint64_t size_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string SieveCachePolicy<Key, Value, KeyHasher>::kClassName("SieveCachePolicy");

} // namespace covered

#endif // SIEVE_CACHE_POLICY_HPP