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

        invalidity_map_.clear();
    }
    
    CacheWrapperBase::~CacheWrapperBase() {}

    bool CacheWrapperBase::isInvalidated(const Key& key) const
    {
        std::map<Key, bool>::const_iterator iter = invalidity_map_.find(key);
        if (iter == invalidity_map_.end())
        {
            return false;
        }
        else
        {
            return iter->second;
        }
    }

    void CacheWrapperBase::invalidate(const Key& key)
    {
        if (invalidity_map_.find(key) != invalidity_map_.end()) // key to be invalidated should already be cached
        {
            invalidity_map_[key] = true;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in invalidity_map_ for invalidate()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            invalidity_map_.insert(std::pair<Key, bool>(key, true));
        }
        return;
    }

    void CacheWrapperBase::validate(const Key& key)
    {
        if (invalidity_map_.find(key) != invalidity_map_.end()) // key to be validated should already be cached
        {
            invalidity_map_[key] = false;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in invalidity_map_ for validate()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            invalidity_map_.insert(std::pair<Key, bool>(key, false));
        }
        return;
    }

    bool CacheWrapperBase::remove(const Key& key)
    {
        Value deleted_value;
        bool is_local_cached = update(key, deleted_value);
        return is_local_cached;
    }

    void CacheWrapperBase::admit(const Key& key, const Value& value)
    {
        admitInternal_(key, value);
        if (invalidity_map_.find(key) == invalidity_map_.end()) // key to be admitted should not be cached
        {
            invalidity_map_.insert(std::pair<Key, bool>(key, false));
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in invalidity_map_ (invalidity: " << (invalidity_map_[key]?"true":"false") << "; map size: " << invalidity_map_.size() << ") for admit()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());

            invalidity_map_[key] = false;
        }
        return;
    }
    
    void CacheWrapperBase::evict(Key& key, Value& value)
    {
        evictInternal_(key, value);
        if (invalidity_map_.find(key) != invalidity_map_.end()) // key to be evicted should already be cached
        {
            invalidity_map_.erase(key);
        }
        else
        {
            std::ostringstream oss;
            oss << "victim key " << key.getKeystr() << " does not exist in invalidity_map_ for evict()";
            Util::dumpWarnMsg(base_instance_name_, oss.str());
            
            // NO need to update invalidity_map_
        }
        return;
    }

    uint32_t CacheWrapperBase::getSize() const
    {
        uint32_t internal_size = getSizeInternal_();
        uint32_t external_size = invalidity_map_.size() * sizeof(bool);
        uint32_t total_size = internal_size + external_size;
        return total_size;
    }
	
    uint32_t CacheWrapperBase::getCapacityBytes() const
    {
        return capacity_bytes_;
    }
}