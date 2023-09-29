#include "edge/cache_server/cache_server_placement_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerPlacementProcessorParam::kClassName("CacheServerPlacementProcessorParam");

    CacheServerPlacementProcessorParam::CacheServerPlacementProcessorParam()
    {
        cache_server_ptr_ = NULL;
        data_request_buffer_ptr_ = NULL;
    }

    CacheServerPlacementProcessorParam::CacheServerPlacementProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for local requests
        data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size);
        assert(data_request_buffer_ptr_ != NULL);
    }

    CacheServerPlacementProcessorParam::~CacheServerPlacementProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerPlacementProcessorParam (e.g., by simulator)

        if (data_request_buffer_ptr_ != NULL)
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerPlacementProcessorParam& CacheServerPlacementProcessorParam::operator=(const CacheServerPlacementProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerPlacementProcessorParam (e.g., by simulator)
        cache_server_ptr_ = other.cache_server_ptr_;

        // Must deep copy the ring buffer of local requests
        if (data_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
        if (other.data_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.data_request_buffer_ptr_->getDefaultElement(), other.data_request_buffer_ptr_->getBufferSize());
            assert(data_request_buffer_ptr_ != NULL);

            *data_request_buffer_ptr_ = *(other.data_request_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerPlacementProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerPlacementProcessorParam::getDataRequestBufferPtr() const
    {
        assert(data_request_buffer_ptr_ != NULL);
        return data_request_buffer_ptr_;
    }
}