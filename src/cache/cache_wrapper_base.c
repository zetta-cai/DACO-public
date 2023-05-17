#include "cache/cache_wrapper_base.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string CacheWrapperBase::LRU_CACHE_NAME("lru");

    const std::string CacheWrapperBase::kClassName("CacheWrapperBase");

    CacheWrapperBase* CacheWrapperBase::getEdgeCache(const std::string& cache_name, const uint32_t& capacity)
    {
        CacheWrapperBase* cache_ptr = NULL;
        if (cache_name == LRU_CACHE_NAME)
        {
            cache_ptr = new LruCacheWrapper(capacity);
        }
        else
        {
            std::ostringstream oss;
            oss << "cache " << cache_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(cache_ptr != NULL);
        return cache_ptr;
    }

    CacheWrapperBase::CacheWrapperBase(const uint32_t& capacity) : capacity_(capacity)
    {
        validity_map_.clear();
    }
    
    CacheWrapperBase::~CacheWrapperBase() {}

    bool CacheWrapperBase::isInvalidated(const uint32_t& key) const
    {
        const std::map<Key, bool>::iterator iter = invalidity_map_.find(key);
        if (iter == invalidity_map_.end())
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    void CacheWrapperBase::invalidate(const uint32_t& key)
    {
        if (invalidity_map_.find(key) != invalidity_map_.end()) // key to be invalidated should already be cached
        {
            invalidity_map_[key] = true;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in invalidity_map_ for invalidate()";
            Util::dumpWarnMsg(kClassName, oss.str());

            invalidity_map_.insert(std::pair<Key, bool>(key, true));
        }
        return;
    }

    void CacheWrapperBase::validate(const uint32_t& key)
    {
        if (invalidity_map_.find(key) != invalidity_map_.end()) // key to be validated should already be cached
        {
            invalidity_map_[key] = false;
        }
        else
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in invalidity_map_ for validate()";
            Util::dumpWarnMsg(kClassName, oss.str());

            invalidity_map_.insert(std::pair<Key, bool>(key, false));
        }
        return;
    }

    bool CacheWrapperBase::remove(const Key& key);
    {
        Value deleted_value();
        bool is_cached = update(key, deleted_value);
        return is_cached;
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
            oss << "key " << key.getKeystr() << " already exists in invalidity_map_ (invalidity: " << invalidity_map_[key]?"true":"false" << "; map size: " << invalidity_map_.size() << ") for admit()";
            Util::dumpWarnMsg(kClassName, oss.str());

            invalidity_map_[key] = false;
        }
        return;
    }
    
    void CacheWrapperBase::evict()
    {
        Key victim_key = evictInternal_();
        if (invalidity_map_.find(victim_key) != invalidity_map_.end()) // key to be evicted should already be cached
        {
            invalidity_map_.erase(victim_key);
        }
        else
        {
            std::ostringstream oss;
            oss << "victim key " << victim_key.getKeystr() << " does not exist in invalidity_map_ for evict()";
            Util::dumpWarnMsg(kClassName, oss.str());
            
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
	
    uint32_t CacheWrapperBase::getCapacity() const
    {
        return capacity_;
    }
}