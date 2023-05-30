/*
 * CacheWrapperBase: base class for general edge cache interfaces.
 *
 * Each individual CacheWrapper needs to override get, update, needIndependentAdmit, admitInternal_, evictInternal_, and getSizeInternal_.
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#ifndef CACHE_WRAPPER_BASE_H
#define CACHE_WRAPPER_BASE_H

#include <map>
#include <string>
#include <set>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class CacheWrapperBase
    {
    public:
        static const std::string LRU_CACHE_NAME;
        static const std::string COVERED_CACHE_NAME;

        static CacheWrapperBase* getEdgeCache(const std::string& cache_name, const uint32_t& capacity_bytes);

        CacheWrapperBase(const uint32_t& capacity_bytes);
        virtual ~CacheWrapperBase();

        // EdgeWrapper checks whether key is invalidated before accessing local edge cache (TODO)
        // True: cached yet invalidated (NO need to access cache)
        // False: uncached (still need to access cache to update metadata) or validated
        bool isInvalidated(const Key& key) const; // For data messages (e.g., local/redirected/global requests)
        void invalidate(const Key& key); // For control messages (e.g., invalidation and admission/eviction)
        void validate(const Key& key); // For control messages (e.g., invalidation and admission/eviction)

        // Return whether key is cached (i.e., cache hit) after get/update/remove
        // NOTE: get() cannot be const due to metadata changes for cached/uncached objects
        // NOTE: remove() just marks the object as deleted, yet not evict the cached object
        virtual bool get(const Key& key, Value& value) = 0;
        virtual bool update(const Key& key, const Value& value) = 0;
        bool remove(const Key& key);

        // If get() / update() / remove() returns false (i.e., key is still uncached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/ LRU-based independent admission policy (i.e., always admit) will always return true, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        virtual bool needIndependentAdmit(const Key& key) = 0;

        // Invoke admitInternal_/evictInternal_ and update invalidity_map_
        void admit(const Key& key, const Value& value);
        void evict();
        
        // In units of bytes
        uint32_t getSize() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)
		uint32_t getCapacityBytes() const;
    private:
        static const std::string kClassName;

        virtual void admitInternal_(const Key& key, const Value& value) = 0;
        virtual Key evictInternal_() = 0;

        // In units of bytes
        virtual uint32_t getSizeInternal_() const = 0;

        const uint32_t capacity_bytes_; // Come from Util::Param

        // NOTE: ONLY write invalidity_map_ for control messages (e.g., requests for invalidation and admission/eviction), while just read it for data messages (local/redirected/global requests)
        // NOTE: as the flag of invalidity can be integrated into cache metadata, we ONLY count the flag instead of key into the total size for capacity limitation (invalidity_map_ is just an implementation trick to avoid hacking each individual cache)
        std::map<Key, bool> invalidity_map_;
    };
}

#endif