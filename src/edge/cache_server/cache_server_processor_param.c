#include "edge/cache_server/cache_server_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerProcessorParam::kClassName("CacheServerProcessorParam");

    CacheServerProcessorParam::CacheServerProcessorParam()
    {
        cache_server_ptr_ = NULL;
        data_request_buffer_ptr_ = NULL;
    }

    CacheServerProcessorParam::CacheServerProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for local requests
        data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size);
        assert(data_request_buffer_ptr_ != NULL);
    }

    CacheServerProcessorParam::~CacheServerProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerProcessorParam (e.g., by simulator)

        if (data_request_buffer_ptr_ != NULL)
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerProcessorParam& CacheServerProcessorParam::operator=(const CacheServerProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerProcessorParam (e.g., by simulator)
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

    CacheServer* CacheServerProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerProcessorParam::getDataRequestBufferPtr() const
    {
        assert(data_request_buffer_ptr_ != NULL);
        return data_request_buffer_ptr_;
    }
}