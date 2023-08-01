/*
 * LruCache: refer to lib/cpp-lru-cache/include/lrucache.hpp.
 *
 * Hack to support update, admit, evict, and size in units of bytes.
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
		typedef std::list<key_value_pair_t>::const_iterator list_const_iterator_t;
		typedef std::unordered_map<Key, list_iterator_t, KeyHasher>::iterator map_iterator_t;
	public:
		LruCache();
		~LruCache();
		
		bool get(const Key& key, Value& value);
		bool update(const Key& key, const Value& value);

		void admit(const Key& key, const Value& value);
		Key getVictimKey() const;
        bool evictIfKeyMatch(const Key& key, Value& value);
		
		bool exists(const Key& key) const;
		
		uint64_t getSizeForCapacity() const;
		
	private:
		static const std::string kClassName;

		// In units of bytes
		uint64_t size_;

		// Store value for each key; Use list index to indicate the most recent access time
		std::list<key_value_pair_t> cache_items_list_;
		// Use list_iterator_t to locate the corresponding list entry in _cache_items_list
		std::unordered_map<Key, list_iterator_t, KeyHasher> cache_items_map_;
	};
} // namespace covered

#endif	/* LRUCACHE_H */