#include "cache/lru/lrucache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
	const std::string LruCache::kClassName("LruCache");

	LruCache::LruCache() : size_(0) {}

	LruCache::~LruCache() {}

	bool LruCache::get(const Key& key, Value& value)
	{
		bool is_local_cached = false;

		map_iterator_t map_iter = cache_items_map_.find(key);
		if (map_iter == cache_items_map_.end()) {
			is_local_cached = false;
		} else {
			list_iterator_t list_iter = map_iter->second;
			assert(list_iter != cache_items_list_.end());
			assert(list_iter->first == key);
			cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, list_iter); // Move the list entry pointed by list_iter to the head of the list (NOT change the memory address of the list entry)
			value = list_iter->second;
			is_local_cached = true;
		}

		return is_local_cached;
	}

	bool LruCache::update(const Key& key, const Value& value) {
		bool is_local_cached = false;

		map_iterator_t map_iter = cache_items_map_.find(key);
		if (map_iter != cache_items_map_.end()) // Previous list and map entry exist
		{
			is_local_cached = true;

			// Update the object with new value
			// Push key-value pair into the head of cache_items_list_, i.e., the key is the most recently used
			cache_items_list_.push_front(key_value_pair_t(key, value));
			size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

			// Remove previous list entry from cache_items_list_
			list_iterator_t prev_list_iter = map_iter->second;
			assert(prev_list_iter != cache_items_list_.end());
			assert(prev_list_iter->first == key);
			uint32_t prev_keysize = prev_list_iter->first.getKeyLength();
			uint32_t prev_valuesize = prev_list_iter->second.getValuesize();
			cache_items_list_.erase(prev_list_iter);
			size_ = Util::uint64Minus(size_, static_cast<uint64_t>(prev_keysize + prev_valuesize));

			// Update map entry with the latest list iterator
			map_iter->second = cache_items_list_.begin();
		}

		return is_local_cached;
	}

	void LruCache::admit(const Key& key, const Value& value)
	{
		map_iterator_t map_iter = cache_items_map_.find(key);
		if (map_iter != cache_items_map_.end()) // Previous list and map entry exist
		{
			uint32_t list_index = static_cast<uint32_t>(std::distance(cache_items_list_.begin(), map_iter->second));
			//uint32_t list_index = static_cast<uint32_t>(map_iter->second - cache_items_list_.begin());
			std::ostringstream oss;
			oss << "key " << key.getKeystr() << " already exists in cache_items_map_ (list index: " << list_index << "; list size: " << cache_items_list_.size() << ") for admit()";
			Util::dumpWarnMsg(kClassName, oss.str());
			return;
		}
		else // No previous list and map entry
		{
			// Insert the object with new value
			// Push key-value pair into the head of cache_items_list_, i.e., the key is the most recently used
			cache_items_list_.push_front(key_value_pair_t(key, value));
			size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

			cache_items_map_.insert(std::pair<Key, list_iterator_t>(key, cache_items_list_.begin()));
			size_ = Util::uint64Add(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(list_iterator_t)));
		}

		return;
	}

	bool LruCache::getVictimKey(Key& key) const
	{
		bool has_victim_key = false;
		if (cache_items_list_.size() > 0)
		{
			// Select victim by LRU
			list_const_iterator_t last_list_iter = cache_items_list_.end();
			last_list_iter--;
			key = last_list_iter->first;

			has_victim_key = true;
		}
		return has_victim_key;
	}
    
	bool LruCache::evictWithGivenKey(const Key& key, Value& value)
	{
		bool is_evict = false;
		
		/*// Select victim by LRU for version check
		Key cur_victim_key;
		bool has_victim_key = getVictimKey(cur_victim_key);
		if (has_victim_key && cur_victim_key == key) // Key matches
		{
			map_iterator_t victim_map_iter = cache_items_map_.find(key);
			assert(victim_map_iter != cache_items_map_.end());
			list_iterator_t victim_list_iter = victim_map_iter->second;
			assert(victim_list_iter != cache_items_list_.end());
			//list_iterator_t last_list_iter = cache_items_list_.end();
			value = victim_list_iter->second;
			uint32_t victim_valuesize = value.getValuesize();

			// Remove the corresponding map entry
			cache_items_map_.erase(key);
			size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(list_iterator_t)));

			// Remove the corresponding list entry
			cache_items_list_.pop_back();
			size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + victim_valuesize));

			is_evict = true;
		}*/

		map_iterator_t victim_map_iter = cache_items_map_.find(key);
		if (victim_map_iter != cache_items_map_.end()) // Key exists
		{
			list_iterator_t victim_list_iter = victim_map_iter->second;
			assert(victim_list_iter != cache_items_list_.end());
			value = victim_list_iter->second;
			uint32_t victim_valuesize = value.getValuesize();

			// Remove the corresponding map entry
			cache_items_map_.erase(key);
			size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + sizeof(list_iterator_t)));

			// Remove the corresponding list entry
			cache_items_list_.pop_back();
			size_ = Util::uint64Minus(size_, static_cast<uint64_t>(key.getKeyLength() + victim_valuesize));

			is_evict = true;
		}

		return is_evict;
	}

	bool LruCache::exists(const Key& key) const
	{
		return cache_items_map_.find(key) != cache_items_map_.end();
	}

	uint64_t LruCache::getSizeForCapacity() const
	{
		return size_;
	}
}