/*
 * CacheServerItem: an item passed between cache server and cache server subthreads (e.g., cache server workers, cache server placement processor, and cache server invalidation processor).
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef CACHE_SERVER_ITEM_H
#define CACHE_SERVER_ITEM_H

#include <string>

#include "message/message_base.h"

namespace covered
{
    class CacheServerItem
    {
    public:
        CacheServerItem();
        CacheServerItem(MessageBase* request_ptr);
        ~CacheServerItem();

        MessageBase* getRequestPtr() const;

        const CacheServerItem& operator=(const CacheServerItem& other);
    private:
        static const std::string kClassName;

        MessageBase* request_ptr_;
    };
}

#endif