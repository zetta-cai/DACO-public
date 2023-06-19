#include "cache/cache_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "cache/lru_cache_wrapper.h"

namespace covered
{
    const std::string CacheWrapperBase::kClassName("CacheWrapperBase");

    CacheWrapperBase* CacheWrapperBase::getEdgeCache(const std::string& cache_name, const uint32_t& capacity_bytes, EdgeParam* edge_param_ptr)
    {
        CacheWrapperBase* cache_ptr = NULL;
        if (cache_name == Param::LRU_CACHE_NAME)
        {
            cache_ptr = new LruCacheWrapper(capacity_bytes, edge_param_ptr);
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

    bool CacheWrapperBase::isValidIfCached(const Key& key) const
    {
        bool is_found = false;
        bool is_valid = validity_map_.isValid(key, is_found);
        if (!is_found) // key is locally cached yet not found in validity_map_, which may due to processing order issue
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " is locally cached yet not found in validity_map_!";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
            assert(is_valid == false);
        }
        return is_valid;
    }

    void CacheWrapperBase::invalidateIfCached(const Key& key)
    {
        bool is_found = false;
        validity_map_.invalidate(key, is_found);
        if (!is_found) // a key locally cached is not found in validity_map_
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
            is_valid = isValidIfCached(key);
        }

        return is_local_cached && is_valid;
    }
    
    bool CacheWrapperBase::update(const Key& key, const Value& value)
    {
        bool is_local_cached = updateInternal_(key, value);

        if (is_local_cached)
        {
            validateIfCached_(key);
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

        if (is_valid)
        {
            validateIfUncached_(key);
        }
        else
        {
            invalidateIfUncached_(key);
        }

        return;
    }
    
    void CacheWrapperBase::evict(Key& key, Value& value)
    {
        evictInternal_(key, value);

        bool is_found = false;
        validity_map_.erase(key, is_found);
        if (!is_found)
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

    void CacheWrapperBase::validateIfCached_(const Key& key)
    {
        bool is_found = false;
        validity_map_.validate(key, is_found);
        if (!is_found) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for validateIfCached()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapperBase::validateIfUncached_(const Key& key)
    {
        bool is_found = false;
        validity_map_.validate(key, is_found);
        if (is_found) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for validateIfUncached_()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapperBase::invalidateIfUncached_(const Key& key)
    {
        bool is_found = false;
        validity_map_.invalidate(key, is_found);
        if (is_found) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for invalidateIfUncached_()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
        }
        return;
    }
}