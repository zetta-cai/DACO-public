/*
 * LfuLocalCache: local edge cache with LFU policy (https://github.com/vpetrigo/caches.git) (thread safe).
 *
 * NOTE: all non-const shared variables in LfuLocalCache should be thread safe.
 * 
 * By Siyuan Sheng (2023.08.07).
 */

#include <string>

#include "cache/lfu/lfu_cache_policy.hpp"
#include "cache/local_cache_base.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class LfuLocalCache : public LocalCacheBase
    {
    public:
        LfuLocalCache(const uint32_t& edge_idx);
        ~LfuLocalCache();

        // (1) Check is cached and access validity

        virtual bool isLocalCached(const Key& key) const override;

        // (2) Access local edge cache

        virtual bool getLocalCache(const Key& key, Value& value) const override;
        virtual bool updateLocalCache(const Key& key, const Value& value) override;

        // (3) Local edge cache management

        virtual bool needIndependentAdmit(const Key& key) const override;
        virtual void admitLocalCache(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKey(Key& key) const override;
        virtual bool evictLocalCacheIfKeyMatch(const Key& key, Value& value) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacity() const override;
    private:
        static const std::string kClassName;

        // (4) Other functions

        virtual void checkPointers_() const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Guarantee the atomicity of local LFU cache and local statistics
        mutable Rwlock* rwlock_for_lfu_local_cache_ptr_;

        // Non-const shared variables
        LFUCachePolicy<Key, Value, KeyHasher>* lfu_cache_ptr_; // Data and metadata for local edge cache
    };
}