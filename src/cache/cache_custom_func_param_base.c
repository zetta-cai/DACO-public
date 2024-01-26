#include "cache/cache_custom_func_param_base.h"

namespace covered
{
    const std::string CacheCustomFuncParamBase::kClassName("CacheCustomFuncParamBase");

    CacheCustomFuncParamBase::CacheCustomFuncParamBase(const bool& need_perkey_lock, const bool& is_perkey_write_lock, const Key& key, const bool& is_local_cache_write_lock)
        : need_perkey_lock_(need_perkey_lock), is_perkey_write_lock_(is_perkey_write_lock), key_(key), is_local_cache_write_lock_(is_local_cache_write_lock)
    {
    }

    CacheCustomFuncParamBase::~CacheCustomFuncParamBase()
    {
    }

    bool CacheCustomFuncParamBase::needPerkeyLock() const
    {
        return need_perkey_lock_;
    }

    bool CacheCustomFuncParamBase::isPerkeyWriteLock() const
    {
        return is_perkey_write_lock_;
    }

    Key CacheCustomFuncParamBase::getKey() const
    {
        return key_;
    }

    bool CacheCustomFuncParamBase::isLocalCacheWriteLock() const
    {
        return is_local_cache_write_lock_;
    }
}