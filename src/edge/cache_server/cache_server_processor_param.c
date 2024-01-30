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

    CacheServerProcessorParam::CacheServerProcessorParam(CacheServerBase* cache_server_ptr, const uint32_t& data_request_buffer_size, const bool& is_polling_based)
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
            cache_server_item_blocking_buffer_ptr_ = new BlockingRingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, CacheServerProcessorParam::isNodeFinish_);
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

    bool CacheServerProcessorParam::isPollingBase() const
    {
        return is_polling_based_;
    }

    CacheServerBase* CacheServerProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    bool CacheServerProcessorParam::push(const CacheServerItem& cache_server_item)
    {
        bool is_successful = false;
        if (is_polling_based_)
        {
            assert(cache_server_item_buffer_ptr_ != NULL);
            is_successful = cache_server_item_buffer_ptr_->push(cache_server_item);
        }
        else
        {
            assert(cache_server_item_blocking_buffer_ptr_ != NULL);
            is_successful = cache_server_item_blocking_buffer_ptr_->push(cache_server_item);
        }
        return is_successful;
    }
    
    bool CacheServerProcessorParam::pop(CacheServerItem& cache_server_item, void* edge_wrapper_ptr)
    {
        bool is_successful = false;
        if (is_polling_based_)
        {
            UNUSED(edge_wrapper_ptr);

            is_successful = cache_server_item_buffer_ptr_->pop(cache_server_item); // Polling-based in a non-blocking manner
        }
        else
        {
            assert(edge_wrapper_ptr != NULL);

            // NOTE: BlockingRingBuffer uses edge wrapper's isNodeRunning as finish condition
            is_successful = cache_server_item_blocking_buffer_ptr_->pop(cache_server_item, edge_wrapper_ptr); // Interruption-based in a blocking manner
        }

        // NOTE: (i) for polling-based ring buffer, is_successful = false means ring buffer is empty, which needs to check if edge node is still running; (ii) for interruption-based ring buffer, is_successful = false means finish condition is satisfied (i.e., edge node is NOT running)
        return is_successful;
    }

    void CacheServerProcessorParam::notifyFinishIfNecessary(void* edge_wrapper_ptr) const
    {
        if (!is_polling_based_)
        {
            assert(edge_wrapper_ptr != NULL);

            cache_server_item_blocking_buffer_ptr_->notifyFinish(edge_wrapper_ptr);
        }
        return;
    }

    bool CacheServerProcessorParam::isNodeFinish_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = static_cast<EdgeWrapperBase*>(edge_wrapper_ptr);
        const bool is_edge_finish = !tmp_edge_wrapper_ptr->isNodeRunning();
        return is_edge_finish;
    }

    // RingBuffer<CacheServerItem>* CacheServerProcessorParam::getCacheServerItemBufferPtr_() const
    // {
    //     assert(is_polling_based_);
    //     assert(cache_server_item_buffer_ptr_ != NULL);
    //     return cache_server_item_buffer_ptr_;
    // }

    // BlockingRingBuffer<CacheServerItem>* CacheServerProcessorParam::getCacheServerItemBlockingItemBufferPtr_() const
    // {
    //     assert(!is_polling_based_);
    //     assert(cache_server_item_blocking_buffer_ptr_ != NULL);
    //     return cache_server_item_blocking_buffer_ptr_;
    // }
}