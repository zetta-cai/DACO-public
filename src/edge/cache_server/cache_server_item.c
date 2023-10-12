#include "edge/cache_server/cache_server_item.h"

#include <assert.h>

namespace covered
{
    const std::string CacheServerItem::kClassName("CacheServerItem");

    CacheServerItem::CacheServerItem()
    {
        request_ptr_ = NULL;
    }

    CacheServerItem::CacheServerItem(MessageBase* request_ptr)
    {
        assert(request_ptr != NULL);
        request_ptr_ = request_ptr;
    }

    CacheServerItem::~CacheServerItem()
    {
        // NOTE: no need to release request_ptr_, which will be released outside CacheServerItem (by the corresponding cache server subthread)
    }

    MessageBase* CacheServerItem::getRequestPtr() const
    {
        assert(request_ptr_ != NULL);
        return request_ptr_;
    }

    const CacheServerItem& CacheServerItem::operator=(const CacheServerItem& other)
    {
        request_ptr_ = other.request_ptr_; // Shallow copy
        return *this;
    }

}