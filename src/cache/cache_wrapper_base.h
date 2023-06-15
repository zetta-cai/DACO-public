/*
 * CacheWrapperBase: base class for general local edge cache interfaces (thread safe).
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
#include "edge/edge_param.h"
#include "lock/rwlock.h"

namespace covered
{
    class CacheWrapperBase
    {
    public:
        static CacheWrapperBase* getEdgeCache(const std::string& cache_name, const uint32_t& capacity_bytes, EdgeParam* edge_param_ptr);

        CacheWrapperBase(const uint32_t& capacity_bytes, EdgeParam* edge_param_ptr);
        virtual ~CacheWrapperBase();

        // EdgeWrapper checks whether key is cached and invalid before accessing local edge cache (TODO)
        // True: cached yet invalid (NO need to access cache)
        // False: uncached (still need to access cache to update metadata) or valid
        bool isCachedAndInvalid(const Key& key) const; // For data messages (e.g., local/redirected/global requests)
        
        void invalidate(const Key& key); // For control messages (e.g., invalidation and admission/eviction)
        void validate(const Key& key); // For control messages (e.g., invalidation and admission/eviction)

        // Return whether key is cached and valid (i.e., local cache hit) after get/update/remove
        bool get(const Key& key, Value& value) const;

        // Return whether key is cached, while both update() and remove() will set validity as true
        // NOTE: update() only updates the object if cached, yet not admit a new one
        // NOTE: remove() only marks the object as deleted if cached, yet not evict it
        bool update(const Key& key, const Value& value);
        bool remove(const Key& key);
        bool isLocalCached(const Key& key) const;

        // If get() or update() or remove() returns false (i.e., key is not cached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/o admission policy (i.e., always admit) will always return true if key is not cached, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        virtual bool needIndependentAdmit(const Key& key) const = 0;

        // Invoke admitInternal_/evictInternal_ and update validity_map_
        void admit(const Key& key, const Value& value, const bool& is_valid);
        void evict(Key& key, Value& value);
        
        // In units of bytes
        uint32_t getSize() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)
		uint32_t getCapacityBytes() const;
    private:
        static const std::string kClassName;

        virtual bool getInternal_(const Key& key, Value& value) const = 0; // Return whether key is cached
        virtual bool updateInternal_(const Key& key, const Value& value) = 0; // Return whether key is cached
        virtual void admitInternal_(const Key& key, const Value& value) = 0;
        virtual void evictInternal_(Key& key, Value& value) = 0;

        // In units of bytes
        virtual uint32_t getSizeInternal_() const = 0;

        // Const shared variables
        const uint32_t capacity_bytes_; // Come from Util::Param

        // CacheWrapperBase only uses edge index to specify base_instance_name_, yet not need to check if edge is running due to no network communication -> no need to maintain edge_param_ptr_
        std::string base_instance_name_;

        // Guarantee the atomicity of validity_map_ (e.g., admit different keys)
        mutable Rwlock* rwlock_for_validity_ptr_;

        // NOTE: write validity_map_ for control messages (e.g., requests for invalidation and admission/eviction), and data messages (local/redirected requests incurring ValidationGetRequest)
        // NOTE: as the flag of validity can be integrated into cache metadata, we ONLY count the flag instead of key into the total size for capacity limitation (validity_map_ is just an implementation trick to avoid hacking each individual cache)
        std::map<Key, bool> validity_map_;
    };
}

#endif