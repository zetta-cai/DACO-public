#include "edge/cache_server/cache_server_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerProcessorParam::kClassName("CacheServerProcessorParam");

    CacheServerProcessorParam::CacheServerProcessorParam()
    {
        cache_server_ptr_ = NULL;
        cache_server_item_buffer_ptr_ = NULL;
    }

    CacheServerProcessorParam::CacheServerProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for metadata update requests
        const bool with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for metadata update requests
        cache_server_item_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, with_multi_providers);
        assert(cache_server_item_buffer_ptr_ != NULL);
    }

    CacheServerProcessorParam::~CacheServerProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerProcessorParam (e.g., by a sub-thread of EdgeWrapper)

        if (cache_server_item_buffer_ptr_ != NULL)
        {
            delete cache_server_item_buffer_ptr_;
            cache_server_item_buffer_ptr_ = NULL;
        }
    }

    const CacheServerProcessorParam& CacheServerProcessorParam::operator=(const CacheServerProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerProcessorParam (e.g., by a sub-thread of EdgeWrapper)
        cache_server_ptr_ = other.cache_server_ptr_;

        // Must deep copy the ring buffer of local requests
        if (cache_server_item_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete cache_server_item_buffer_ptr_;
            cache_server_item_buffer_ptr_ = NULL;
        }
        if (other.cache_server_item_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.cache_server_item_buffer_ptr_->withMultiProviders();
            assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for metadata update requests

            cache_server_item_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.cache_server_item_buffer_ptr_->getDefaultElement(), other.cache_server_item_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(cache_server_item_buffer_ptr_ != NULL);

            *cache_server_item_buffer_ptr_ = *(other.cache_server_item_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerProcessorParam::getCacheServerItemBufferPtr() const
    {
        assert(cache_server_item_buffer_ptr_ != NULL);
        return cache_server_item_buffer_ptr_;
    }
}