#include "edge/cache_server/cache_server_worker_param.h"

#include "common/util.h"

namespace covered
{
    // CacheServerWorkerItem

    CacheServerWorkerItem::CacheServerWorkerItem()
    {
        local_request_ptr_ = NULL;
        client_addr_ = NetworkAddr();
    }

    CacheServerWorkerItem::CacheServerWorkerItem(MessageBase* local_request_ptr, const NetworkAddr& client_addr)
    {
        assert(local_request_ptr != NULL);
        local_request_ptr_ = local_request_ptr;
        client_addr_ = client_addr;
    }

    CacheServerWorkerItem::~CacheServerWorkerItem()
    {
        // NOTE: no need to release local_request_ptr_, which will be released outside CacheServerWorkerItem (by the corresponding cache server worker thread)
    }

    MessageBase* CacheServerWorkerItem::getLocalRequestPtr() const
    {
        return local_request_ptr_;
    }
    
    NetworkAddr CacheServerWorkerItem::getClientAddr() const
    {
        return client_addr_;
    }

    CacheServerWorkerItem& CacheServerWorkerItem::operator=(const CacheServerWorkerItem& other)
    {
        local_request_ptr_ = other.local_request_ptr_;
        client_addr_ = other.client_addr_;
        return *this;
    }

    // CacheServerWorkerParam

    const uint32_t CacheServerWorkerParam::LOCAL_REQUEST_BUFFER_CAPACITY = 1000;
    const std::string CacheServerWorkerParam::kClassName("CacheServerWorkerParam");

    CacheServerWorkerParam::CacheServerWorkerParam()
    {
        edge_wrapper_ptr_ = NULL;
        local_cache_server_worker_idx_ = 0;
        local_request_buffer_ptr_ = NULL;
    }

    CacheServerWorkerParam::CacheServerWorkerParam(EdgeWrapper* edge_wrapper_ptr, uint32_t local_cache_server_worker_idx, const uint32_t& buffer_capacity)
    {
        edge_wrapper_ptr_ = edge_wrapper_ptr;
        local_cache_server_worker_idx_ = local_cache_server_worker_idx;
        
        // Allocate ring buffer for local requests
        local_request_buffer_ptr_ = new RingBuffer<CacheServerWorkerItem>(CacheServerWorkerItem(), buffer_capacity);
        assert(local_request_buffer_ptr_ != NULL);
    }

    CacheServerWorkerParam::~CacheServerWorkerParam()
    {
        // NOTE: no need to release edge_wrapper_ptr_, which will be released outside CacheServerWorkerParam (e.g., by simulator)

        if (local_request_buffer_ptr_ != NULL)
        {
            delete local_request_buffer_ptr_;
            local_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerWorkerParam& CacheServerWorkerParam::operator=(const CacheServerWorkerParam& other)
    {
        // Shallow copy is okay, as edge_wrapper_ptr_ is maintained outside CacheServerWorkerParam (e.g., by simulator)
        edge_wrapper_ptr_ = other.edge_wrapper_ptr_;

        local_cache_server_worker_idx_ = other.local_cache_server_worker_idx_;

        // Must deep copy the ring buffer of local requests
        if(local_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete local_request_buffer_ptr_;
            local_request_buffer_ptr_ = NULL;
        }
        if (other.local_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            local_request_buffer_ptr_ = new RingBuffer<CacheServerWorkerItem>(other.local_request_buffer_ptr_->getDefaultElement(), other.local_request_buffer_ptr_->getCapacity());
            assert(local_request_buffer_ptr_ != NULL);

            *local_request_buffer_ptr_ = *(other.local_request_buffer_ptr_);
        }

        return *this;
    }

    EdgeWrapper* CacheServerWorkerParam::getEdgeWrapperPtr() const
    {
        return edge_wrapper_ptr_;
    }

    uint32_t CacheServerWorkerParam::getLocalCacheServerWorkerIdx() const
    {
        return local_cache_server_worker_idx_;
    }

    RingBuffer<CacheServerWorkerItem>* CacheServerWorkerParam::getLocalRequestBufferPtr() const
    {
        return local_request_buffer_ptr_;
    }
}