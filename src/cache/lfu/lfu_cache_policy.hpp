/*
 * LFUCachePolicy: refer to lib/caches/include/lfu_cache_policy.hpp.
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * Note: emplace_hint/erase will NOT invalidate previous iterators for non-continuous data stuctures (e.g., multimap/map/set/list) vs. continuous ones (e.g., vector/queue/array).
 * 
 * By Siyuan Sheng (2023.08.07).
 */

#ifndef LFU_CACHE_POLICY_HPP
#define LFU_CACHE_POLICY_HPP

#include <cstddef>
#include <iostream>
#include <map>
#include <unordered_map>

#include <cache_policy.hpp>

#include "common/util.h"

namespace covered
{
    /**
     * \brief LFU (Least frequently used) cache policy
     * \details LFU policy in the case of replacement removes the least frequently used
     * element.
     *
     * Each access to an element in the cache increments internal counter (frequency) that
     * represents how many times that particular key has been accessed by someone. When a
     * replacement has to occur the LFU policy just takes a look onto keys' frequencies
     * and remove the least used one. E.g. cache of two elements where `A` has been accessed
     * 10 times and `B` – only 2. When you want to add a key `C` the LFU policy returns `B`
     * as a replacement candidate.
     * \tparam Key Type of a key a policy works with
     */
    template <typename Key, typename Value, typename KeyHasher>
    class LFUCachePolicy : public caches::ICachePolicy<Key>
    {
    public:
        using lfu_iterator = typename std::multimap<std::uint32_t, std::pair<Key, Value>>::iterator;
        typedef typename std::unordered_map<Key, lfu_iterator, KeyHasher>::iterator map_iterator_t;

        LFUCachePolicy() : caches::ICachePolicy<Key>()
        {
            frequency_storage.clear();
            lfu_storage.clear();
            size_ = 0;
        }

        virtual ~LFUCachePolicy() override
        {
        }

        bool exists(const Key& key) const
        {
            return lfu_storage.find(key) != lfu_storage.end();
        }

        bool get(const Key& key, Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = lfu_storage.find(key);
            if (map_iter == lfu_storage.end()) {
                is_local_cached = false;
            } else {
                // Get previous frequency and value
                lfu_iterator list_iter = map_iter->second;
                assert(list_iter != frequency_storage.end());
                assert(list_iter->second.first == key);
                uint32_t prev_frequency = list_iter->first;
                value = list_iter->second.second;

                // Erase previous list entry from frequency_storage
                frequency_storage.erase(list_iter);

                // Add new list entry into frequency_storage
                lfu_iterator new_list_iter = frequency_storage.emplace_hint(frequency_storage.cend(), std::make_pair(prev_frequency + 1, std::make_pair(key, value)));

                // Update key indexing with new list iterator
                map_iter->second = new_list_iter;
                
                is_local_cached = true;
            }

            return is_local_cached;
        }

        bool update(const Key& key, const Value& value)
        {
            bool is_local_cached = false;

            map_iterator_t map_iter = lfu_storage.find(key);
            if (map_iter != lfu_storage.end()) // Previous list and map entry exist
            {
                is_local_cached = true;

                // Get previous frequency and value
                lfu_iterator list_iter = map_iter->second;
                assert(list_iter != frequency_storage.end());
                assert(list_iter->second.first == key);
                uint32_t prev_frequency = list_iter->first;
                //Value original_value = list_iter->second.second;

                // Erase previous list entry from frequency_storage
                uint32_t prev_keysize = list_iter->second.first.getKeyLength();
                uint32_t prev_valuesize = list_iter->second.second.getValuesize();
                frequency_storage.erase(list_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(sizeof(uint32_t) + prev_keysize + prev_valuesize));

                // Add new list entry into frequency_storage
                lfu_iterator new_list_iter = frequency_storage.emplace_hint(frequency_storage.cend(), std::make_pair(prev_frequency + 1, std::make_pair(key, value)));
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(sizeof(uint32_t) + key.getKeyLength() + value.getValuesize()));

                // Update key indexing with new list iterator
                map_iter->second = new_list_iter;
            }

            return is_local_cached;
        }

        void admit(const Key& key, const Value& value)
        {
            map_iterator_t map_iter = lfu_storage.find(key);
            if (map_iter != lfu_storage.end()) // Previous list and map entry exist
            {
                std::ostringstream oss;
                oss << "key " << key.getKeyDebugstr() << " already exists in lfu_storage (list size: " << frequency_storage.size() << ") for admit()";
                Util::dumpWarnMsg(kClassName, oss.str());
                return;
            }
            else // No previous list and map entry
            {
                // Insert the new key-value object with initial frequency of 1
                constexpr std::uint32_t INIT_VAL = 1;
                lfu_iterator new_list_iter = frequency_storage.emplace_hint(frequency_storage.cbegin(), INIT_VAL, std::make_pair(key, value));
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(sizeof(uint32_t) + key.getKeyLength() + value.getValuesize()));

                // Insert new list iterator for key indexing
                lfu_storage.insert(std::make_pair(key, new_list_iter));
                size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(lfu_iterator)));
            }

            return;
        }

        bool getVictimKey(Key& key) const
        {
            bool has_victim_key = false;

            if (frequency_storage.size() > 0)
            {
                // Get the least frequency used key
                key = frequency_storage.cbegin()->second.first;

                has_victim_key = true;
            }

            return has_victim_key;
        }

        bool evictWithGivenKey(const Key& key, Value& value)
        {   
            bool is_evict = false;
            
            /*// Select victim by LFU for version check
            Key cur_victim_key;
            bool has_victim_key = getVictimKey(cur_victim_key);
            if (has_victim_key && cur_victim_key == key) // Key matches
            {
                // Get victim value
                map_iterator_t victim_map_iter = lfu_storage.find(key);
                assert(victim_map_iter != lfu_storage.end());
                lfu_iterator victim_list_iter = victim_map_iter->second;
                assert(victim_list_iter != frequency_storage.end());
                assert(victim_list_iter->second.first == key);
                value = victim_list_iter->second.second;

                // Remove the corresponding map entry
                lfu_storage.erase(victim_map_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(lfu_iterator)));

                // Remove the corresponding list entry
                frequency_storage.erase(victim_list_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(sizeof(uint32_t) + key.getKeyLength() + value.getValuesize()));

                is_evict = true;
            }*/

            // Get victim value
            map_iterator_t victim_map_iter = lfu_storage.find(key);
            if (victim_map_iter != lfu_storage.end()) // Key exists
            {
                lfu_iterator victim_list_iter = victim_map_iter->second;
                assert(victim_list_iter != frequency_storage.end());
                assert(victim_list_iter->second.first == key);
                value = victim_list_iter->second.second;

                // Remove the corresponding map entry
                lfu_storage.erase(victim_map_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(lfu_iterator)));

                // Remove the corresponding list entry
                frequency_storage.erase(victim_list_iter);
                size_ = Util::uint64Minus(size_, static_cast<uint64_t>(sizeof(uint32_t) + key.getKeyLength() + value.getValuesize()));

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
            Util::dumpErrorMsg(kClassName, "Insert() is not supported for LFU cache policy");
            exit(1);

            /*
            constexpr std::uint32_t INIT_VAL = 1;
            // all new value initialized with the frequency 1
            lfu_storage[key] =
                frequency_storage.emplace_hint(frequency_storage.cbegin(), INIT_VAL, key);
            */
        }

        void Touch(const Key &key) override
        {
            Util::dumpErrorMsg(kClassName, "Touch() is not supported for LFU cache policy");
            exit(1);

            /*
            // get the previous frequency value of a key
            auto elem_for_update = lfu_storage[key];
            auto updated_elem = std::make_pair(elem_for_update->first + 1, elem_for_update->second);
            // update the previous value
            frequency_storage.erase(elem_for_update);
            lfu_storage[key] =
                frequency_storage.emplace_hint(frequency_storage.cend(), std::move(updated_elem));
            */
        }

        void Erase(const Key &key) noexcept override
        {
            Util::dumpErrorMsg(kClassName, "Erase() is not supported for LFU cache policy");
            exit(1);

            /*
            frequency_storage.erase(lfu_storage[key]);
            lfu_storage.erase(key);
            */
        }

        const Key &ReplCandidate() const noexcept override
        {
            Util::dumpErrorMsg(kClassName, "ReplCandidate() is not supported for LFU cache policy");
            exit(1);

            /*
            // at the beginning of the frequency_storage we have the
            // least frequency used value
            return frequency_storage.cbegin()->second;
            */
        }

    private:
        static const std::string kClassName;

        // NOTE: std::multimap is a sorted list allowing duplicate keys (i.e., frequencies here)
        std::multimap<std::uint32_t, std::pair<Key, Value>> frequency_storage; // Key-value cache ordered by frequency
        std::unordered_map<Key, lfu_iterator, KeyHasher> lfu_storage; // Key indexing for quick lookup

        uint64_t size_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string LFUCachePolicy<Key, Value, KeyHasher>::kClassName("LFUCachePolicy");

} // namespace covered

#endif // LFU_CACHE_POLICY_HPP
