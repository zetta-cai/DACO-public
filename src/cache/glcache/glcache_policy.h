/*
 * GLCachePolicy: refer to lib/glcache/micro-implementation/libCacheSim/cache/eviction/GLCache/GLCache.c, yet reimplement in C++ and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + impractical assumption of in-request next access time + not-make-installed header files).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint; also hack to support object-oriented programming in C++ by adding a prefix of "cpp_" for each non-static global variable and function in original C files to avoid conflicting definition.
 * 
 * NOTE: GLCache is a coarse-grained cache replacement policy due to segment-level merge-based eviction.
 * 
 * By Siyuan Sheng (2024.01.05).
 */

#ifndef GLCACHE_H
#define GLCACHE_H

#include <string>
#include <unordered_map>

#include <libCacheSim/cache.h> // Siyuan: lib/glcache/micro-implementation/build/include/libCacheSim/cache.h

namespace covered
{
	// Forward declaration
	class GLCachePolicy;
}

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class GLCachePolicy
    {
    public:
		GLCachePolicy(const uint64_t& capacity_bytes);
		~GLCachePolicy();

        bool exists(const Key& key) const;
		
		bool get(const Key& key, Value& value);
		bool update(const Key& key, const Value& value);

		void admit(const Key& key, const Value& value);
        void evictNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims);
		
		uint64_t getSizeForCapacity() const;
		
	private:
		static const std::string kClassName;

		cache_t* cache_metadata_ptr_;

		// In units of bytes
		uint64_t size_;
    };
}

#endif