/*
 * LocalCacheBase: base class of local edge cache (thread safe).
 * 
 * NOTE: all non-const shared variables in CacheWrapperBase should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.30).
 */

#ifndef LOCAL_CACHE_BASE_H
#define LOCAL_CACHE_BASE_H

#include <string>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class LocalCacheBase
    {
    public:
        static LocalCacheBase* getLocalCacheByCacheName(const std::string& cache_name, const uint32_t& edge_idx);

        LocalCacheBase(const uint32_t& edge_idx);
        virtual ~LocalCacheBase();

        // (1) Check is cached and access validity

        virtual bool isLocalCached(const Key& key) const = 0;

        // (2) Access local edge cache

        virtual bool getLocalCache(const Key& key, Value& value) const = 0; // Return whether key is cached
        virtual bool updateLocalCache(const Key& key, const Value& value) = 0; // Return whether key is cached

        // (3) Local edge cache management

        // If get() or update() or remove() in CacheWrapper returns false (i.e., key is not cached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/o admission policy (i.e., always admit) will always return true if key is not cached, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        virtual bool needIndependentAdmit(const Key& key) const = 0;

        virtual void admitLocalCache(const Key& key, const Value& value) = 0;
        virtual void evictLocalCache(Key& key, Value& value) = 0;

        // (4) Other functions
        
        // In units of bytes
        virtual uint32_t getSizeForCapacity() const = 0; // Get size of data and metadata for local edge cache

        
    private:
        static const std::string kClassName;

        // (4) Other functions

        virtual void checkPointers_() const = 0;        

        // Member variables

        std::string base_instance_name_; // Const shared variable

        // NOTE: we will use a single read-write lock for thread safety of local edge cache, including concurrent accesses of cached objects and local statistics with fair comparison
        // NOTE: we do NOT hack each cache with fine-grained locking to avoid extensive complexity
    };
}

#endif