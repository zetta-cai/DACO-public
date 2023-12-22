#include "edge/cache_server/cache_server_worker_param.h"

#include "common/util.h"

namespace covered
{
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
        const bool with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for local/redirected data requests
        data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, with_multi_providers);
        assert(data_request_buffer_ptr_ != NULL);
    }

    CacheServerWorkerParam::~CacheServerWorkerParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerWorkerParam (e.g., by a sub-thread of EdgeWrapper)

        if (data_request_buffer_ptr_ != NULL)
        {
            // Release messages in remaining cache server items
            std::vector<CacheServerItem> remaining_elements;
            data_request_buffer_ptr_->getAllToRelease(remaining_elements);
            for (uint32_t i = 0; i < remaining_elements.size(); i++)
            {
                MessageBase* tmp_remaining_message_ptr = remaining_elements[i].getRequestPtr();
                assert(tmp_remaining_message_ptr != NULL);
                delete tmp_remaining_message_ptr;
                tmp_remaining_message_ptr = NULL;
            }
            
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
    }

    const CacheServerWorkerParam& CacheServerWorkerParam::operator=(const CacheServerWorkerParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerWorkerParam (e.g., by a sub-thread of EdgeWrapper)
        cache_server_ptr_ = other.cache_server_ptr_;

        local_cache_server_worker_idx_ = other.local_cache_server_worker_idx_;

        // Must deep copy the ring buffer of local requests
        if (data_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            // TODO: may need to release messages in remaining cache server items
            
            delete data_request_buffer_ptr_;
            data_request_buffer_ptr_ = NULL;
        }
        if (other.data_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.data_request_buffer_ptr_->withMultiProviders();
            assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for local/redirected data requests

            data_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.data_request_buffer_ptr_->getDefaultElement(), other.data_request_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(data_request_buffer_ptr_ != NULL);

            *data_request_buffer_ptr_ = *(other.data_request_buffer_ptr_);
        }

        return *this;
    }

    CacheServer* CacheServerWorkerParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    uint32_t CacheServerWorkerParam::getLocalCacheServerWorkerIdx() const
    {
        return local_cache_server_worker_idx_;
    }

    RingBuffer<CacheServerItem>* CacheServerWorkerParam::getDataRequestBufferPtr() const
    {
        assert(data_request_buffer_ptr_ != NULL);
        return data_request_buffer_ptr_;
    }
}