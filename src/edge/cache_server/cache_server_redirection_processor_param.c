#include "edge/cache_server/cache_server_redirection_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerRedirectionProcessorParam::kClassName("CacheServerRedirectionProcessorParam");

    CacheServerRedirectionProcessorParam::CacheServerRedirectionProcessorParam()
    {
        cache_server_ptr_ = NULL;
        data_request_buffer_ptr_ = NULL;
    }

    CacheServerRedirectionProcessorParam::CacheServerRedirectionProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for redirected data requests
        const bool with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for redirected data requests
        data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, with_multi_providers);
        assert(data_request_buffer_ptr_ != NULL);
    }

    CacheServerRedirectionProcessorParam::~CacheServerRedirectionProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerRedirectionProcessorParam (e.g., by a sub-thread of EdgeWrapper)

        if (data_request_buffer_ptr_ != NULL)
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerRedirectionProcessorParam& CacheServerRedirectionProcessorParam::operator=(const CacheServerRedirectionProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerRedirectionProcessorParam (e.g., by a sub-thread of EdgeWrapper)
        cache_server_ptr_ = other.cache_server_ptr_;

        // Must deep copy the ring buffer of local requests
        if (data_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
        if (other.data_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.data_request_buffer_ptr_->withMultiProviders();
            assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for redirected data requests

            data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.data_request_buffer_ptr_->getDefaultElement(), other.data_request_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(data_request_buffer_ptr_ != NULL);

            *data_request_buffer_ptr_ = *(other.data_request_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerRedirectionProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerRedirectionProcessorParam::getDataRequestBufferPtr() const
    {
        assert(data_request_buffer_ptr_ != NULL);
        return data_request_buffer_ptr_;
    }
}