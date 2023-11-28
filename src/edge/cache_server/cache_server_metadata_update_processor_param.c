#include "edge/cache_server/cache_server_metadata_update_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerMetadataUpdateProcessorParam::kClassName("CacheServerMetadataUpdateProcessorParam");

    CacheServerMetadataUpdateProcessorParam::CacheServerMetadataUpdateProcessorParam()
    {
        cache_server_ptr_ = NULL;
        control_request_buffer_ptr_ = NULL;
    }

    CacheServerMetadataUpdateProcessorParam::CacheServerMetadataUpdateProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for metadata update requests
        const bool with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for metadata update requests
        control_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, with_multi_providers);
        assert(control_request_buffer_ptr_ != NULL);
    }

    CacheServerMetadataUpdateProcessorParam::~CacheServerMetadataUpdateProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerMetadataUpdateProcessorParam (e.g., by a sub-thread of EdgeWrapper)

        if (control_request_buffer_ptr_ != NULL)
        {
            delete control_request_buffer_ptr_;
            control_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerMetadataUpdateProcessorParam& CacheServerMetadataUpdateProcessorParam::operator=(const CacheServerMetadataUpdateProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerMetadataUpdateProcessorParam (e.g., by a sub-thread of EdgeWrapper)
        cache_server_ptr_ = other.cache_server_ptr_;

        // Must deep copy the ring buffer of local requests
        if (control_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete control_request_buffer_ptr_;
            control_request_buffer_ptr_ = NULL;
        }
        if (other.control_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.control_request_buffer_ptr_->withMultiProviders();
            assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for metadata update requests

            control_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.control_request_buffer_ptr_->getDefaultElement(), other.control_request_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(control_request_buffer_ptr_ != NULL);

            *control_request_buffer_ptr_ = *(other.control_request_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerMetadataUpdateProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerMetadataUpdateProcessorParam::getControlRequestBufferPtr() const
    {
        assert(control_request_buffer_ptr_ != NULL);
        return control_request_buffer_ptr_;
    }
}