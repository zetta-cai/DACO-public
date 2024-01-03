/*
 * FIFOCachePolicy: refer to lib/caches/include/fifo_cache_policy.hpp.
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * Note: emplace_hint/erase will NOT invalidate previous iterators for non-continuous data stuctures (e.g., multimap/map/set/list) vs. continuous ones (e.g., vector/queue/array).
 * 
 * By Siyuan Sheng (2024.01.01).
 */

#ifndef FIFO_CACHE_POLICY_HPP
#define FIFO_CACHE_POLICY_HPP

#include <list>
#include <unordered_map>

#include <cache_policy.hpp>

#include "common/util.h"

namespace covered
{
    /**
     * \brief FIFO (First in, first out) cache policy
     * \details FIFO policy in the case of replacement removes the first added element.
     *
     * That is, consider the following key adding sequence:
     * ```
     * A -> B -> C -> ...
     * ```
     * In the case a cache reaches its capacity, the FIFO replacement candidate policy
     * returns firstly added element `A`. To show that:
     * ```
     * # New key: X
     * Initial state: A -> B -> C -> ...
     * Replacement candidate: A
     * Final state: B -> C -> ... -> X -> ...
     * ```
     * An so on, the next candidate will be `B`, then `C`, etc.
     * \tparam Key Type of a key a policy works with
     */
    template <typename Key, typename Value, typename KeyHasher>
    class FIFOCachePolicy : public caches::ICachePolicy<Key>
    {
    public:
        using fifo_iterator = typename std::list<std::pair<Key, Value>>::iterator;
        using fifo_const_iterator = typename std::list<std::pair<Key, Value>>::const_iterator;
        typedef typename std::unordered_map<Key, fifo_iterator, KeyHasher>::iterator map_iterator_t;
        typedef typename std::unordered_map<Key, fifo_iterator, KeyHasher>::const_iterator map_const_iterator_t;

        FIFOCachePolicy() : caches::ICachePolicy<Key>()
        {
            fifo_queue_.clear();
            key_lookup_.clear();
            size_ = 0;
        }

        virtual ~FIFOCachePolicy() override
        {
        }

        bool exists(const Key& key) const
        {
            return key_lookup_.find(key) != key_lookup_.end();
        }

        bool get(const Key& key, Value& value) const
        {
            bool is_local_cached = false;

            map_const_iterator_t map_iter = key_lookup_.find(key);
            if (map_iter == key_lookup_.end())
            {
                is_local_cached = false;
            }
            else
            {
                // NOTE: NO metadata to update for FIFO

                // Get value
                fifo_const_iterator list_iter = map_iter->second;
                assert(list_iter != fifo_queue_.end());
                assert(list_iter->first == key);
                value = list_iter->second;

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
                // NOTE: NO metadata to update for FIFO

                // Get previous value
                fifo_iterator list_iter = map_iter->second;
                assert(list_iter != fifo_queue_.end());
                assert(list_iter->first == key);
                uint32_t prev_valuesize = list_iter->second.getValuesize();
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(prev_valuesize));

                // Update value
                list_iter->second = value;
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
                oss << "key " << key.getKeystr() << " already exists in key_lookup_ (list size: " << fifo_queue_.size() << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                // Insert a new key-value entry to the head of the list
                fifo_queue_.emplace_front(key, value);
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

                // Insert new list iterator for key indexing
                map_iter = key_lookup_.insert(std::pair(key, fifo_queue_.begin())).first;
                assert(map_iter != key_lookup_.end());
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(fifo_iterator)));
            }

            return;
        }

        bool getVictimKey(Key& key) const
        {
            bool has_victim_key = false;

            if (fifo_queue_.size() > 0)
            {
                // Get the tail of the list
                key = fifo_queue_.back().first;

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
                fifo_iterator victim_list_iter = victim_map_iter->second;
                assert(victim_list_iter != fifo_queue_.end());
                assert(victim_list_iter->first == key);
                value = victim_list_iter->second;

                // Remove the corresponding map entry
                key_lookup_.erase(victim_map_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(fifo_iterator)));

                // Remove the corresponding list entry
                fifo_queue_.erase(victim_list_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

                is_evict = true;
            }

            return is_evict;
        }

        uint64_t getSizeForCapacity() const
        {
            return size_;
        }

        void Insert(const Key &key) override
        {
            Util::dumpErrorMsg(kClassName, "Insert() is not supported for FIFO cache policy");
            exit(1);

            /*
            fifo_queue.emplace_front(key);
            key_lookup[key] = fifo_queue.begin();
            */
        }

        // handle request to the key-element in a cache
        void Touch(const Key &key) noexcept override
        {
            Util::dumpErrorMsg(kClassName, "Touch() is not supported for FIFO cache policy");
            exit(1);

            /*
            // nothing to do here in the FIFO strategy
            (void)key;
            */
        }

        // handle element deletion from a cache
        void Erase(const Key &key) noexcept override
        {
            Util::dumpErrorMsg(kClassName, "Erase() is not supported for FIFO cache policy");
            exit(1);

            /*
            auto element = key_lookup[key];
            fifo_queue.erase(element);
            key_lookup.erase(key);
            */
        }

        // return a key of a replacement candidate
        const Key &ReplCandidate() const noexcept override
        {
            Util::dumpErrorMsg(kClassName, "ReplCandidate() is not supported for FIFO cache policy");
            exit(1);

            /*
            return fifo_queue.back();
            */
        }

    private:
        static const std::string kClassName;

        std::list<std::pair<Key, Value>> fifo_queue_; // Key-value pairs in FIFO order
        std::unordered_map<Key, fifo_iterator, KeyHasher> key_lookup_; // Key indexing for quick lookup

        uint64_t size_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string FIFOCachePolicy<Key, Value, KeyHasher>::kClassName("FIFOCachePolicy");

} // namespace covered

#endif // FIFO_CACHE_POLICY_HPP