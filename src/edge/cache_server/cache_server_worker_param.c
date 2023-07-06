#include "edge/cache_server/cache_server_worker_param.h"

#include "common/util.h"

namespace covered
{
    // CacheServerWorkerItem

    CacheServerWorkerItem::CacheServerWorkerItem()
    {
        data_request_ptr_ = NULL;
    }

    CacheServerWorkerItem::CacheServerWorkerItem(MessageBase* data_request_ptr)
    {
        assert(data_request_ptr != NULL);
        data_request_ptr_ = data_request_ptr;
    }

    CacheServerWorkerItem::~CacheServerWorkerItem()
    {
        // NOTE: no need to release data_request_ptr_, which will be released outside CacheServerWorkerItem (by the corresponding cache server worker thread)
    }

    MessageBase* CacheServerWorkerItem::getDataRequestPtr() const
    {
        return data_request_ptr_;
    }

    CacheServerWorkerItem& CacheServerWorkerItem::operator=(const CacheServerWorkerItem& other)
    {
        data_request_ptr_ = other.data_request_ptr_; // Shallow copy
        return *this;
    }

    // CacheServerWorkerParam

    const std::string CacheServerWorkerParam::kClassName("CacheServerWorkerParam");

    CacheServerWorkerParam::CacheServerWorkerParam()
    {
        cache_server_ptr_ = NULL;
        local_cache_server_worker_idx_ = 0;
        data_request_buffer_ptr_ = NULL;
    }

    CacheServerWorkerParam::CacheServerWorkerParam(CacheServer* cache_server_ptr, const uint32_t& local_cache_server_worker_idx, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        local_cache_server_worker_idx_ = local_cache_server_worker_idx;
        
        // Allocate ring buffer for local requests
        data_request_buffer_ptr_ = new RingBuffer<CacheServerWorkerItem>(CacheServerWorkerItem(), data_request_buffer_size);
        assert(data_request_buffer_ptr_ != NULL);
    }

    CacheServerWorkerParam::~CacheServerWorkerParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerWorkerParam (e.g., by simulator)

        if (data_request_buffer_ptr_ != NULL)
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerWorkerParam& CacheServerWorkerParam::operator=(const CacheServerWorkerParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerWorkerParam (e.g., by simulator)
        cache_server_ptr_ = other.cache_server_ptr_;

        local_cache_server_worker_idx_ = other.local_cache_server_worker_idx_;

        // Must deep copy the ring buffer of local requests
        if(data_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
        if (other.data_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            data_request_buffer_ptr_ = new RingBuffer<CacheServerWorkerItem>(other.data_request_buffer_ptr_->getDefaultElement(), other.data_request_buffer_ptr_->getBufferSize());
            assert(data_request_buffer_ptr_ != NULL);

            *data_request_buffer_ptr_ = *(other.data_request_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerWorkerParam::getCacheServerPtr() const
    {
        return cache_server_ptr_;
    }

    uint32_t CacheServerWorkerParam::getLocalCacheServerWorkerIdx() const
    {
        return local_cache_server_worker_idx_;
    }

    RingBuffer<CacheServerWorkerItem>* CacheServerWorkerParam::getDataRequestBufferPtr() const
    {
        return data_request_buffer_ptr_;
    }
}