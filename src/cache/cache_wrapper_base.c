#include "cache/cache_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "cache/lru_cache_wrapper.h"

namespace covered
{
    const std::string CacheWrapperBase::kClassName("CacheWrapperBase");

    CacheWrapperBase* CacheWrapperBase::getEdgeCache(const std::string& cache_name, EdgeParam* edge_param_ptr)
    {
        CacheWrapperBase* cache_ptr = NULL;
        if (cache_name == Param::LRU_CACHE_NAME)
        {
            cache_ptr = new LruCacheWrapper(edge_param_ptr);
        }
        else
        {
            std::ostringstream oss;
            oss << "local edge cache " << cache_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(cache_ptr != NULL);
        return cache_ptr;
    }

    CacheWrapperBase::CacheWrapperBase(EdgeParam* edge_param_ptr) : validity_map_(edge_param_ptr)
    {
        // Differentiate local edge cache in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        base_instance_name_ = oss.str();
    }
    
    CacheWrapperBase::~CacheWrapperBase() {}

    bool CacheWrapperBase::isValidKeyForLocalCachedObject(const Key& key) const
    {
        bool is_exist = false;
        bool is_valid = validity_map_.isValidFlagForKey(key, is_exist);
        if (!is_exist) // key is locally cached yet not found in validity_map_, which may due to processing order issue
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " is locally cached yet not found in validity_map_!";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
            assert(is_valid == false);
        }
        return is_valid;
    }

    void CacheWrapperBase::invalidateKeyForLocalCachedObject(const Key& key)
    {
        bool is_exist = false;
        validity_map_.invalidateFlagForKey(key, is_exist);
        if (!is_exist) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for invalidateIfCached()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }

    bool CacheWrapperBase::get(const Key& key, Value& value) const
    {
        bool is_local_cached = getInternal_(key, value); // Still need to update local statistics if key is cached yet invalid

        bool is_valid = false;
        if (is_local_cached)
        {
            is_valid = isValidKeyForLocalCachedObject(key);
        }

        return is_local_cached && is_valid;
    }
    
    bool CacheWrapperBase::update(const Key& key, const Value& value)
    {
        bool is_local_cached = updateInternal_(key, value);

        if (is_local_cached)
        {
            validateKeyForLocalCachedObject_(key);
        }

        return is_local_cached;
    }

    bool CacheWrapperBase::remove(const Key& key)
    {
        // No need to acquire a write lock for validity_map_, which will be done in update()

        Value deleted_value;
        bool is_local_cached = update(key, deleted_value);
        return is_local_cached;
    }

    void CacheWrapperBase::admit(const Key& key, const Value& value, const bool& is_valid)
    {
        admitInternal_(key, value);

        if (is_valid) // w/o writes
        {
            validateKeyForLocalUncachedObject_(key);
        }
        else // w/ writes
        {
            invalidateKeyForLocalUncachedObject_(key);
        }

        return;
    }
    
    void CacheWrapperBase::evict(Key& key, Value& value)
    {
        evictInternal_(key, value);

        bool is_exist = false;
        validity_map_.eraseFlagForKey(key, is_exist);
        if (!is_exist)
        {
            std::ostringstream oss;
            oss << "victim key " << key.getKeystr() << " does not exist in validity_map_ for evict()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        
        }

        return;
    }

    uint32_t CacheWrapperBase::getSizeForCapacity() const
    {
        uint32_t local_edge_cache_size = getSizeInternal_();
        uint32_t validity_map_size = validity_map_.getSizeForCapacity();
        uint32_t total_size = local_edge_cache_size + validity_map_size;
        return total_size;
    }

    void CacheWrapperBase::validateKeyForLocalCachedObject_(const Key& key)
    {
        bool is_exist = false;
        validity_map_.validateFlagForKey(key, is_exist);
        if (!is_exist) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for validateIfCached()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapperBase::validateKeyForLocalUncachedObject_(const Key& key)
    {
        bool is_exist = false;
        validity_map_.validateFlagForKey(key, is_exist);
        if (is_exist) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for validateIfUncached_()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapperBase::invalidateKeyForLocalUncachedObject_(const Key& key)
    {
        bool is_exist = false;
        validity_map_.invalidateFlagForKey(key, is_exist);
        if (is_exist) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for invalidateIfUncached_()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }
}