/*
 * CustomFuncParamBase: base class for custom function parameters.
 *
 * By Siyuan Sheng (2024.01.10).
 */

#ifndef CUSTOM_FUNC_PARAM_BASE
#define CUSTOM_FUNC_PARAM_BASE

#include <string>

#include "common/key.h"

namespace covered
{
    class CustomFuncParamBase
    {
    public:
        CustomFuncParamBase(const bool& need_perkey_lock, const bool& is_perkey_write_lock, const Key& key, const bool& is_local_cache_write_lock);
        virtual ~CustomFuncParamBase();

        bool needPerkeyLock() const;
        bool isPerkeyWriteLock() const;
        Key getKey() const;
        bool isLocalCacheWriteLock() const;
    private:
        static const std::string kClassName;

        const bool need_perkey_lock_;
        const bool is_perkey_write_lock_;
        const Key key_;
        const bool is_local_cache_write_lock_;
    };
}

#endif