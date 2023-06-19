/*
 * CacheWrapperBase: base class for general local edge cache interfaces (thread safe).
 *
 * Each individual CacheWrapper needs to override get, update, needIndependentAdmit, admitInternal_, evictInternal_, and getSizeInternal_.
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#ifndef CACHE_WRAPPER_BASE_H
#define CACHE_WRAPPER_BASE_H

#include <string>

#include "cache/validity_map.h"
#include "common/key.h"
#include "common/value.h"
#include "edge/edge_param.h"

namespace covered
{
    class CacheWrapperBase
    {
    public:
        static CacheWrapperBase* getEdgeCache(const std::string& cache_name, const uint32_t& capacity_bytes, EdgeParam* edge_param_ptr);

        CacheWrapperBase(EdgeParam* edge_param_ptr);
        virtual ~CacheWrapperBase();

        virtual bool isLocalCached(const Key& key) const = 0;
        bool isValidIfCached(const Key& key) const;
        void invalidateIfCached(const Key& key); // For invalidation control requests

        // Return whether key is cached and valid (i.e., local cache hit) after get/update/remove
        bool get(const Key& key, Value& value) const;

        // Return whether key is cached, while both update() and remove() will set validity as true
        // NOTE: update() only updates the object if cached, yet not admit a new one
        // NOTE: remove() only marks the object as deleted if cached, yet not evict it
        bool update(const Key& key, const Value& value);
        bool remove(const Key& key);

        // If get() or update() or remove() returns false (i.e., key is not cached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/o admission policy (i.e., always admit) will always return true if key is not cached, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        virtual bool needIndependentAdmit(const Key& key) const = 0;

        // Invoke admitInternal_/evictInternal_ and update validity_map_
        void admit(const Key& key, const Value& value, const bool& is_valid);
        void evict(Key& key, Value& value);
        
        // In units of bytes
        uint32_t getSizeForCapacity() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)
    private:
        static const std::string kClassName;

        void validateIfCached_(const Key& key); // For local put/del requests invoked by update() and remove()
        void validateIfUncached_(const Key& key); // For local get/put/del requests invoked by admit() w/ writes
        void invalidateIfUncached_(const Key& key); // For local get/put/del requests invoked by admit() w/o writes

        virtual bool getInternal_(const Key& key, Value& value) const = 0; // Return whether key is cached
        virtual bool updateInternal_(const Key& key, const Value& value) = 0; // Return whether key is cached
        virtual void admitInternal_(const Key& key, const Value& value) = 0;
        virtual void evictInternal_(Key& key, Value& value) = 0;
        virtual bool existsInternal_(const Key& key) const = 0; // Return whether key is cached

        // In units of bytes
        virtual uint32_t getSizeInternal_() const = 0; // Get size of data and metadata for local edge cache

        // CacheWrapperBase only uses edge index to specify base_instance_name_, yet not need to check if edge is running due to no network communication -> no need to maintain edge_param_ptr_
        std::string base_instance_name_;

        ValidityMap validity_map_; // Maintain per-key validity flag for local edge cache (thread safe)
    };
}

#endif