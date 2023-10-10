#include "edge/cache_server/cache_server_item.h"

namespace covered
{
    const std::string CacheServerItem::kClassName("CacheServerItem");

    CacheServerItem::CacheServerItem()
    {
        data_request_ptr_ = NULL;
    }

    CacheServerItem::CacheServerItem(MessageBase* data_request_ptr)
    {
        assert(data_request_ptr != NULL);
        data_request_ptr_ = data_request_ptr;
    }

    CacheServerItem::~CacheServerItem()
    {
        // NOTE: no need to release data_request_ptr_, which will be released outside CacheServerItem (by the corresponding cache server subthread)
    }

    MessageBase* CacheServerItem::getRequestPtr() const
    {
        assert(data_request_ptr_ != NULL);
        return data_request_ptr_;
    }

    const CacheServerItem& CacheServerItem::operator=(const CacheServerItem& other)
    {
        data_request_ptr_ = other.data_request_ptr_; // Shallow copy
        return *this;
    }

}