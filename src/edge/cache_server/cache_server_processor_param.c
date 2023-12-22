#include "edge/cache_server/cache_server_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerProcessorParam::kClassName("CacheServerProcessorParam");

    CacheServerProcessorParam::CacheServerProcessorParam()
    {
        is_polling_based_ = false;
        cache_server_ptr_ = NULL;
        cache_server_item_buffer_ptr_ = NULL;
        cache_server_item_blocking_buffer_ptr_ = NULL;
    }

    CacheServerProcessorParam::CacheServerProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size, const bool& is_polling_based)
    {
        is_polling_based_ = is_polling_based;
        cache_server_ptr_ = cache_server_ptr;
        cache_server_item_buffer_ptr_ = NULL;
        cache_server_item_blocking_buffer_ptr_ = NULL;
        
        if (is_polling_based)
        {
            // Allocate polling-based ring buffer for data/control requests
            const bool with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for data/control requests
            cache_server_item_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, with_multi_providers);
            assert(cache_server_item_buffer_ptr_ != NULL);
        }
        else
        {
            // Allocate interruption-based ring buffer for data/control requests
            cache_server_item_blocking_buffer_ptr_ = new BlockingRingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, CacheServerProcessorParam::isNodeFinish);
            assert(cache_server_item_blocking_buffer_ptr_ != NULL);
        }
    }

    CacheServerProcessorParam::~CacheServerProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerProcessorParam (e.g., by a sub-thread of EdgeWrapper)

        if (is_polling_based_ && cache_server_item_buffer_ptr_ != NULL)
        {
            assert(cache_server_item_blocking_buffer_ptr_ == NULL);

            // Release messages in remaining cache server items
            std::vector<CacheServerItem> remaining_elements;
            cache_server_item_buffer_ptr_->getAllToRelease(remaining_elements);
            for (uint32_t i = 0; i < remaining_elements.size(); i++)
            {
                MessageBase* tmp_remaining_message_ptr = remaining_elements[i].getRequestPtr();
                assert(tmp_remaining_message_ptr != NULL);
                delete tmp_remaining_message_ptr;
                tmp_remaining_message_ptr = NULL;
            }

            delete cache_server_item_buffer_ptr_;
            cache_server_item_buffer_ptr_ = NULL;
        }

        if (!is_polling_based_ && cache_server_item_blocking_buffer_ptr_ != NULL)
        {
            assert(cache_server_item_buffer_ptr_ == NULL);

            // Release messages in remaining cache server items
            std::vector<CacheServerItem> remaining_elements;
            cache_server_item_blocking_buffer_ptr_->getAllToRelease(remaining_elements);
            for (uint32_t i = 0; i < remaining_elements.size(); i++)
            {
                MessageBase* tmp_remaining_message_ptr = remaining_elements[i].getRequestPtr();
                assert(tmp_remaining_message_ptr != NULL);
                delete tmp_remaining_message_ptr;
                tmp_remaining_message_ptr = NULL;
            }

            delete cache_server_item_blocking_buffer_ptr_;
            cache_server_item_blocking_buffer_ptr_ = NULL;
        }
    }

    const CacheServerProcessorParam& CacheServerProcessorParam::operator=(const CacheServerProcessorParam& other)
    {
        is_polling_based_ = other.is_polling_based_;

        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerProcessorParam (e.g., by a sub-thread of EdgeWrapper)
        cache_server_ptr_ = other.cache_server_ptr_;

        if (is_polling_based_)
        {
            assert(cache_server_item_blocking_buffer_ptr_ == NULL);

            // Must deep copy the ring buffer of data/control requests
            if (cache_server_item_buffer_ptr_ != NULL) // Release original ring buffer if any
            {
                // TODO: may need to release messages in remaining cache server items

                delete cache_server_item_buffer_ptr_;
                cache_server_item_buffer_ptr_ = NULL;
            }
            if (other.cache_server_item_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
            {
                const bool other_with_multi_providers = other.cache_server_item_buffer_ptr_->withMultiProviders();
                assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for data/control requests

                cache_server_item_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.cache_server_item_buffer_ptr_->getDefaultElement(), other.cache_server_item_buffer_ptr_->getBufferSize(), other_with_multi_providers);
                assert(cache_server_item_buffer_ptr_ != NULL);

                *cache_server_item_buffer_ptr_ = *(other.cache_server_item_buffer_ptr_);
            }
        }
        else
        {
            assert(cache_server_item_buffer_ptr_ == NULL);

            // Must deep copy the interruption-based ring buffer of data/control requests
            if (cache_server_item_blocking_buffer_ptr_ != NULL) // Release original interruption-based ring buffer if any
            {
                // TODO: may need to release messages in remaining cache server items

                delete cache_server_item_blocking_buffer_ptr_;
                cache_server_item_blocking_buffer_ptr_ = NULL;
            }
            if (other.cache_server_item_blocking_buffer_ptr_ != NULL) // Deep copy other's interruption-based ring buffer if any
            {
                cache_server_item_blocking_buffer_ptr_ = new BlockingRingBuffer<CacheServerItem>(other.cache_server_item_blocking_buffer_ptr_->getDefaultElement(), other.cache_server_item_blocking_buffer_ptr_->getBufferSize(), other.cache_server_item_blocking_buffer_ptr_->getFinishConditionFunc());
                assert(cache_server_item_blocking_buffer_ptr_ != NULL);

                *cache_server_item_blocking_buffer_ptr_ = *(other.cache_server_item_blocking_buffer_ptr_);
            }
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
        assert(is_polling_based_);
        assert(cache_server_item_buffer_ptr_ != NULL);
        return cache_server_item_buffer_ptr_;
    }

    BlockingRingBuffer<CacheServerItem>* CacheServerProcessorParam::getCacheServerItemBlockingItemBufferPtr() const
    {
        assert(!is_polling_based_);
        assert(cache_server_item_blocking_buffer_ptr_ != NULL);
        return cache_server_item_blocking_buffer_ptr_;
    }

    bool CacheServerProcessorParam::isNodeFinish(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        EdgeWrapper* tmp_edge_wrapper_ptr = static_cast<EdgeWrapper*>(edge_wrapper_ptr);
        const bool is_edge_finish = !tmp_edge_wrapper_ptr->isNodeRunning();
        return is_edge_finish;
    }
}