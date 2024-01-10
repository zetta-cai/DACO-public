#include "cache/custom_func_param_base.h"

namespace covered
{
    const std::string CustomFuncParamBase::kClassName("CustomFuncParamBase");

    CustomFuncParamBase::CustomFuncParamBase(const bool& need_perkey_lock, const bool& is_perkey_write_lock, const Key& key, const bool& is_local_cache_write_lock)
        : need_perkey_lock_(need_perkey_lock), is_perkey_write_lock_(is_perkey_write_lock), key_(key), is_local_cache_write_lock_(is_local_cache_write_lock)
    {
    }

    CustomFuncParamBase::~CustomFuncParamBase()
    {
    }

    bool CustomFuncParamBase::needPerkeyLock() const
    {
        return need_perkey_lock_;
    }

    bool CustomFuncParamBase::isPerkeyWriteLock() const
    {
        return is_perkey_write_lock_;
    }

    Key CustomFuncParamBase::getKey() const
    {
        return key_;
    }

    bool CustomFuncParamBase::isLocalCacheWriteLock() const
    {
        return is_local_cache_write_lock_;
    }
}