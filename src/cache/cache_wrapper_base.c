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

    CacheWrapperBase::CacheWrapperBase(const uint32_t& capacity_bytes, EdgeParam* edge_param_ptr) : capacity_bytes_(capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " " << edge_param_ptr->getEdgeIdx();
        base_instance_name_ = oss.str();

        validity_map_.clear();
    }
    
    CacheWrapperBase::~CacheWrapperBase() {}

    bool CacheWrapperBase::isCachedAndInvalid(const Key& key) const
    {
        // Acquire a read lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock_shared())
            {
                break;
            }
        }

        std::map<Key, bool>::const_iterator iter = validity_map_.find(key);
        bool is_cached_and_invalid = false;
        if (iter == validity_map_.end())
        {
            is_cached_and_invalid = false;
        }
        else
        {
            is_cached_and_invalid = !iter->second; // validity = true means not invalid
        }

        rwlock_for_validity_.unlock_shared();
        return is_cached_and_invalid;
    }

    void CacheWrapperBase::invalidate(const Key& key)
    {
        // Acquire a write lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock())
            {
                break;
            }
        }

        if (validity_map_.find(key) != validity_map_.end()) // key to be invalidated should already be cached
        {
            validity_map_[key] = false;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for invalidate()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            validity_map_.insert(std::pair<Key, bool>(key, false));
        }

        rwlock_for_validity_.unlock();
        return;
    }

    void CacheWrapperBase::validate(const Key& key)
    {
        // Acquire a write lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock())
            {
                break;
            }
        }

        if (validity_map_.find(key) != validity_map_.end()) // key to be validated should already be cached
        {
            validity_map_[key] = true;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for validate()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            validity_map_.insert(std::pair<Key, bool>(key, true));
        }

        rwlock_for_validity_.unlock();
        return;
    }

    bool CacheWrapperBase::get(const Key& key, Value& value) const
    {
        // Acquire a read lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock_shared())
            {
                break;
            }
        }

        bool is_local_cached = getInternal_(key, value); // Still need to update local statistics if key is cached yet invalid
        bool is_valid = false;
        if (is_local_cached)
        {
            std::map<Key, bool>::const_iterator iter = validity_map_.find(key);
            assert(iter != validity_map_.end()); // key must be cached
            is_valid = iter->second;
        }

        rwlock_for_validity_.unlock_shared();
        return is_local_cached && is_valid;
    }
    
    bool CacheWrapperBase::update(const Key& key, const Value& value)
    {
        // Acquire a write lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock())
            {
                break;
            }
        }

        bool is_local_cached = updateInternal_(key, value);
        if (is_local_cached)
        {
            validity_map_[key] = true;
        }

        rwlock_for_validity_.unlock();
        return is_local_cached;
    }

    bool CacheWrapperBase::remove(const Key& key)
    {
        // No need to acquire a write lock which will be done in update()

        Value deleted_value;
        bool is_local_cached = update(key, deleted_value);
        return is_local_cached;
    }

    bool CacheWrapperBase::isLocalCached(const Key& key) const
    {
        // Acquire a read lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock_shared())
            {
                break;
            }
        }

        bool is_local_cached = validity_map_.find(key) != validity_map_.end();

        rwlock_for_validity_.unlock_shared();
        return is_local_cached;
    }

    void CacheWrapperBase::admit(const Key& key, const Value& value)
    {
        // Acquire a write lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock())
            {
                break;
            }
        }

        admitInternal_(key, value);
        if (validity_map_.find(key) == validity_map_.end()) // key to be admitted should not be cached
        {
            validity_map_.insert(std::pair<Key, bool>(key, true));
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ (validity: " << (validity_map_[key]?"true":"false") << "; map size: " << validity_map_.size() << ") for admit()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            validity_map_[key] = true;
        }

        rwlock_for_validity_.unlock();
        return;
    }
    
    void CacheWrapperBase::evict(Key& key, Value& value)
    {
        // Acquire a write lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock())
            {
                break;
            }
        }

        evictInternal_(key, value);
        if (validity_map_.find(key) != validity_map_.end()) // key to be evicted should already be cached
        {
            validity_map_.erase(key);
        }
        else
        {
            std::ostringstream oss;
            oss << "victim key " << key.getKeystr() << " does not exist in validity_map_ for evict()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
            
            // NO need to update validity_map_
        }

        rwlock_for_validity_.unlock();
        return;
    }

    uint32_t CacheWrapperBase::getSize() const
    {
        // Acquire a read lock before accessing any shared variable
        while (true)
        {
            if (rwlock_for_validity_.try_lock_shared())
            {
                break;
            }
        }

        uint32_t internal_size = getSizeInternal_();
        uint32_t external_size = validity_map_.size() * sizeof(bool);
        uint32_t total_size = internal_size + external_size;

        rwlock_for_validity_.unlock_shared();
        return total_size;
    }
	
    uint32_t CacheWrapperBase::getCapacityBytes() const
    {
        // No need to acquire a write lock as capacity_bytes_ is a const variable
        return capacity_bytes_;
    }
}