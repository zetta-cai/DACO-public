/*
 * LruCache: refer to lib/cpp-lru-cache/include/lrucache.hpp.
 *
 * Hack to limite cache capacity (in unit of bytes).
 * 
 * By Siyuan Sheng (2023.05.03).
 */

#ifndef LRUCACHE_H
#define	LRUCACHE_H

#include <list> // std::list
#include <unordered_map> // std::unordered_map

#include "common/key.h"
#include "common/value.h"

namespace covered {
	class LruCache {
	private:
		typedef std::pair<Key, Value> key_value_pair_t;
		typedef std::list<key_value_pair_t>::iterator list_iterator_t;
		typedef std::unordered_map<Key, list_iterator_t>::iterator map_iterator_t;
	public:
		LruCache(const uint32_t& capacity);
		
		void put(const Key& key, const Value& value);
		
		bool get(const Key& key, Value& value);
		
		bool exists(const Key& key) const;
		
		uint32_t getSize() const;
		
	private:
		static const std::string kClassName;

		// In units of bytes
		const uint32_t capacity_;
		uint32_t size_;

		// Store value for each key; Use list index to indicate the most recent access time
		std::list<key_value_pair_t> cache_items_list_;
		// Use list_iterator_t to locate the corresponding list entry in _cache_items_list
		std::unordered_map<Key, list_iterator_t> cache_items_map_;
	};
} // namespace covered

#endif	/* LRUCACHE_H */