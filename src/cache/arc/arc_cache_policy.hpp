/*
 * ArcCachePolicy: refer to Algorithm ARC(c) in original paper and lib/s3fifo/libCacheSim/libCacheSim/cache/eviction/ARC.c, yet directly reimplement in C++ due to simplicity of ARC to escape the dependency on libcachesim and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + insufficient cache size usage calculation).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: NO need to use optimized data structures, as system bottleneck is network propagation latency in geo-distributed edge settings.
 * 
 * By Siyuan Sheng (2024.01.18).
 */

#ifndef ARC_CACHE_POLICY_HPP
#define ARC_CACHE_POLICY_HPP

#ifndef MAX
//#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
//#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#include <list>
#include <unordered_map>

#include "common/util.h"

namespace covered
{
    template <typename Key, typename Value>
    class ArcGhostItem
    {
    public:
        ArcGhostItem(): key_(), lru_id_(0), is_ghost_(true)
        {
        }

        ArcGhostItem(const Key& key, const int& lru_id) : is_ghost_(true)
        {
            key_ = key;
            lru_id_ = lru_id;
        }

        Key getKey() const
        {
            return key_;
        }

        int getLruId() const
        {
            return lru_id_;
        }

        bool isGhost() const
        {
            return is_ghost_;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(key_.getKeyLength() + sizeof(int) + sizeof(bool));
        }

        const ArcGhostItem& operator=(const ArcGhostItem& other)
        {
            if (this != &other)
            {
                key_ = other.key_;
                lru_id_ = other.lru_id_;
                is_ghost_ = other.is_ghost_;
            }
            return *this;
        }
    private:
        Key key_;
        int lru_id_; // In LRU 1 or LRU 2
        bool is_ghost_; // In ghost LRU or data LRU
    };

    template <typename Key, typename Value>
    class ArcDataItem : public ArcGhostItem<Key, Value>
    {
    public:
        ArcDataItem() : ArcGhostItem<Key, Value>(), value_(), is_ghost_(false)
        {
        }

        ArcDataItem(const Key& key, const Value& value, const int& lru_id) : ArcGhostItem<Key, Value>(key, lru_id)
        {
            value_ = value;
            is_ghost_ = false;
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
            return static_cast<uint64_t>(ArcGhostItem::getSizeForCapacity() + value_.getValuesize());
        }

        const ArcDataItem& operator=(const ArcDataItem& other)
        {
            if (this != &other)
            {
                ArcGhostItem::operator=(other);
                value_ = other.value_;
            }
            return *this;
        }
    private:
        Value value_;
    };

    template <typename Key, typename Value>
    class ArcKeyLookupIter
    {
    public:
        ArcKeyLookupIter(const std::list<ArcDataItem<Key, Value>>::iterator& data_list_iter) : lru_id_(data_list_iter->getLruId()), is_ghost_(false)
        {
            data_list_iter_ =  data_list_iter;
        }

        ArcKeyLookupIter(const std::list<ArcGhostItem<Key, Value>>::iterator& ghost_list_iter) : lru_id_(ghost_list_iter->getLruId()), is_ghost_(true)
        {
            ghost_list_iter_ = ghost_list_iter;
        }

        int getLruId() const
        {
            return lru_id_;
        }

        bool isGhost() const
        {
            return is_ghost_;
        }

        const std::list<ArcDataItem<Key, Value>>::iterator& getDataListIterConstRef() const
        {
            return data_list_iter_;
        }

        const std::list<ArcGhostItem<Key, Value>>::iterator& getGhostListIterConstRef() const
        {
            return ghost_list_iter_;
        }

        const uint64_t getSizeForCapacity() const
        {
            return static_cast<uint64_t>(sizeof(std::list<ArcDataItem<Key, Value>>::iterator)); // Siyuan: ONLY count cache size usage of one iterator (we maintain a boolean and two iterators just for impl trick)
        }

        const ArcGhostItem<Key, Value>& operator=(const ArcGhostItem<Key, Value>& other)
        {
            if (this != &other)
            {
                is_ghost_ = other.is_ghost_;
                data_list_iter_ = other.data_list_iter_;
                ghost_list_iter_ = other.ghost_list_iter_;
            }
            return *this;
        }
    private:
        int lru_id_;
        bool is_ghost_;
        std::list<ArcDataItem<Key, Value>>::iterator data_list_iter_;
        std::list<ArcGhostItem<Key, Value>>::iterator ghost_list_iter_;
    };

    template <typename Key, typename Value, typename KeyHasher>
    class ArcCachePolicy
    {
    public:
        typedef typename std::list<ArcDataItem<Key, Value>>::iterator datalist_iterator_t;
        typedef typename std::list<ArcDataItem<Key, Value>>::const_iterator datalist_const_iterator_t;
        typedef typename std::list<ArcGhostItem<Key, Value>>::iterator ghostlist_iterator_t;
        typedef typename std::list<ArcGhostItem<Key, Value>>::const_iterator ghostlist_const_iterator_t;
        typedef typename std::unordered_map<Key, ArcKeyLookupIter<Key, Value>, KeyHasher>::iterator lookupmap_iterator_t;
        typedef typename std::unordered_map<Key, ArcKeyLookupIter<Key, Value>, KeyHasher>::const_iterator lookupmap_const_iterator_t;

        ArcCachePolicy(const uint64_t& capacity_bytes) : capacity_bytes_(capacity_bytes)
        {
            L1_data_size_ = 0;
            L2_data_size_ = 0;
            L1_ghost_size_ = 0;
            L2_ghost_size_ = 0;
            L1_data_list_.clear();
            L1_ghost_list_.clear();
            L2_data_list_.clear();
            L2_ghost_list_.clear();

            p_ = 0;
            curr_obj_in_L1_ghost_ = false;
            curr_obj_in_L2_ghost_ = false;
            vtime_last_req_in_ghost_ = -1;
            reqseq_ = 0;

            key_lookup_.clear();
            size_ = 0;
        }

        ~ArcCachePolicy()
        {
            L1_data_list_.clear();
            L1_ghost_list_.clear();
            L2_data_list_.clear();
            L2_ghost_list_.clear();
            
            key_lookup_.clear();
        }

        bool exists(const Key& key) const
        {
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
                    else {
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
						value = data_list_iter_const_ref->getValue();

                        // move to LRU2 (no need to update cache size usage due to no change)
						// Remove from data LRU 1
						//decreaseDataAndTotalSize_(data_list_iter_const_ref->getSizeForCapacity(), lru_id);
						L1_data_list_.erase(data_list_iter_const_ref);
						// Insert into data LRU 2
						ArcDataItem tmp_item(key, value, 2);
						//increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), 2);
						L2_data_list_.push_front(tmp_item);

						// update lookup table (no need to update cache size usage due to no change)
						ArcLookupIter tmp_lookup_iter(L2_data_list_.begin());
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
						value = data_list_iter_const_ref->getValue();

                        // move to LRU2 head (no need to update cache size usage due to no change)
						ArcDataItem tmp_item = *data_list_iter_const_ref;
						// Remove from data LRU 2
						//decreaseDataAndTotalSize_(data_list_iter_const_ref->getSizeForCapacity(), lru_id);
						L2_data_list_.erase(data_list_iter_const_ref);
						// Insert into head of data LRU 2
						//increaseDataAndTotalSize_(tmp_item.getSizeForCapacity(), lru_id);
						L2_data_list_.push_front(tmp_item);

						// update lookup table (no need to update cache size usage due to no change)
						ArcLookupIter tmp_lookup_iter(L2_data_list_.begin());
						assert(tmp_lookup_iter.getLruId() == 2);
						assert(!tmp_lookup_iter.isGhost());
						lookup_map_iter->second = tmp_lookup_iter;
                    }
                }
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

        void decreaseGhostAndTotalSize_(const uint64_t& decrease_bytes, const int& lru_id)
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

		void increaseDataAndTotalSize_(const uint64_t& increase_bytes, const int& lru_id)
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

        void decreaseTotalSize_(const uint64_t& decrease_bytes)
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

		void increaseTotalSize_(const uint64_t& increase_bytes)
		{
			size_ += increase_bytes;
			return;
		}

    private:
        static const std::string kClassName;

        // L1_data is T1 in the paper, L1_ghost is B1 in the paper
        uint64_t L1_data_size_;
        uint64_t L2_data_size_;
        uint64_t L1_ghost_size_;
        uint64_t L2_ghost_size_;

        std::list<ArcDataItem<Key, Value>> L1_data_list_;
        std::list<ArcGhostItem<Key, Value>> L1_ghost_list_;

        std::list<ArcDataItem<Key, Value>> L2_data_list_;
        std::list<ArcGhostItem<Key, Value>> L2_ghost_list_;

        double p_; // Siyuan: max bytes of L1 data and ghose lists
        bool curr_obj_in_L1_ghost_;
        bool curr_obj_in_L2_ghost_;
        int64_t vtime_last_req_in_ghost_;
        int64_t reqseq_; // Siyuan: virtual/logical timestamp of the incoming request
        //request_t *req_local; // Siyuan: NOT used by ARC in src/cache/arc/ARC.c

        std::unordered_map<Key, ArcKeyLookupIter, KeyHasher> key_lookup_; // Key indexing for quick lookup
        uint64_t size_; // in units of bytes
        const uint64_t capacity_bytes_; // in units of bytes
    };

    template <typename Key, typename Value, typename KeyHasher>
    const std::string ArcCachePolicy<Key, Value, KeyHasher>::kClassName("ArcCachePolicy");

} // namespace covered

#endif // SIEVE_CACHE_POLICY_HPP
