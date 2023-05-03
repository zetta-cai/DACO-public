#include "cache/cpp-lru-cache/lrucache.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
	const std::string LruCache::kClassName("LruCache");

	LruCache::LruCache(const uint32_t& capacity) : size_(0), capacity_(capacity) {}

	void LruCache::put(const Key& key, const Value& value) {
		// Push key-value pair into the head of cache_items_list_, i.e., the key is the most recently used
		cache_items_list_.push_front(key_value_pair_t(key, value));
		size_ += (key.getKeystr().length() + value.getValuesize());

		map_iterator_t map_iter = cache_items_map_.find(key);
		if (map_iter != cache_items_map_.end()) // Previous list and map entry exist
		{
			// Remove previous list entry from cache_items_list_
			list_iterator_t prev_list_iter = map_iter->second;
			assert(prev_list_iter != cache_items_list_.end());
			uint32_t prev_keysize = prev_list_iter->first.getKeystr().length();
			uint32_t prev_valuesize = prev_list_iter->second.getValuesize();
			cache_items_list_.erase(prev_list_iter);
			size_ -= (prev_keysize + prev_valuesize);

			// Update map entry with the latest list iterator
			map_iter->second = cache_items_list_.begin();
		}
		else // No previous list and map entry
		{
			cache_items_map_.insert(std::pair<Key, list_iterator_t>(key, cache_items_list_.begin()));
			size_ += (key.getKeystr().length() + sizeof(list_iterator_t));
		}
		
		while (size_ > capacity_) // Exceed cache capacity
		{
			// Select victim by LRU
			list_iterator_t last_list_iter = cache_items_list_.end();
			last_list_iter--;
			Key victim_key = last_list_iter->first;
			uint32_t victim_valuesize = last_list_iter->second.getValuesize();

			// Remove the corresponding map entry
			cache_items_map_.erase(victim_key);
			size_ -= (victim_key.getKeystr().length() + sizeof(list_iterator_t));

			// Remove the correpsonding list entry
			cache_items_list_.pop_back();
			size_ -= (victim_key.getKeystr().length() + victim_valuesize);
		}

		return;
	}

	bool LruCache::get(const Key& key, Value& value)
	{
		map_iterator_t map_iter = cache_items_map_.find(key);
		if (map_iter == cache_items_map_.end()) {
			return false;
		} else {
			// Note: insert/erase/splice will NOT invalidate previous iterators for non-continuous data stuctures (e.g., map/set/list) vs. continuous ones (e.g., vector/queue/array)
			list_iterator_t list_iter = map_iter->second;
			cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, list_iter); // Move the list entry pointed by list_iter to the head of the list (NOT change the memory address of the list entry)
			value = list_iter->second;
			return true;
		}
	}

	bool LruCache::exists(const Key& key) const
	{
		return cache_items_map_.find(key) != cache_items_map_.end();
	}

	uint32_t LruCache::getSize() const
	{
		return size_;
	}
}