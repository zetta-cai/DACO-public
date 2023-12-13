#include "edge/synchronization_server/synchronization_server_param.h"

#include <assert.h>

namespace covered
{
    const std::string SynchronizationServerParam::kClassName = "SynchronizationServerParam";

    SynchronizationServerParam::SynchronizationServerParam()
    {
        edge_wrapper_ptr_ = NULL;
        synchronization_server_item_buffer_ptr_ = NULL;
    }

    SynchronizationServerParam::SynchronizationServerParam(EdgeWrapper* edge_wrapper_ptr, const uint32_t& data_request_buffer_size)
    {
        edge_wrapper_ptr_ = edge_wrapper_ptr;

        // Allocate ring buffer for remote victim synchronization
        const bool with_multi_providers = true; // Multiple providers (including cache server and beacon server) for remote victim synchronization
        synchronization_server_item_buffer_ptr_ = new RingBuffer<SynchronizationServerItem>(SynchronizationServerItem(), data_request_buffer_size, with_multi_providers);
        assert(synchronization_server_item_buffer_ptr_ != nullptr);
    }

    SynchronizationServerParam::~SynchronizationServerParam()
    {
        // NOTE: no need to release edge_wrapper_ptr_, which will be released outside SynchronizationServerParam (e.g., by the thread of EdgeWrapper)
        
        if (synchronization_server_item_buffer_ptr_ != nullptr)
        {
            delete synchronization_server_item_buffer_ptr_;
            synchronization_server_item_buffer_ptr_ = nullptr;
        }
    }

    const SynchronizationServerParam& SynchronizationServerParam::operator=(const SynchronizationServerParam& other)
    {
        // Shallow copy is okay, as edge_wrapper_ptr_ is maintained outside SynchronizationServerParam (e.g., by the thread of EdgeWrapper)
        edge_wrapper_ptr_ = other.edge_wrapper_ptr_;

        // Must deep copy the ring buffer of remote victim synchronization
        if (synchronization_server_item_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete synchronization_server_item_buffer_ptr_;
            synchronization_server_item_buffer_ptr_ = NULL;
        }
        if (other.synchronization_server_item_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.synchronization_server_item_buffer_ptr_->withMultiProviders();
            assert(other_with_multi_providers); // Multiple providers (including cache server and beacon server) for remote victim synchronization

            synchronization_server_item_buffer_ptr_ = new RingBuffer<SynchronizationServerItem>(other.synchronization_server_item_buffer_ptr_->getDefaultElement(), other.synchronization_server_item_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(synchronization_server_item_buffer_ptr_ != NULL);

            *synchronization_server_item_buffer_ptr_ = *(other.synchronization_server_item_buffer_ptr_);
        }

        return *this;
    }

    EdgeWrapper* SynchronizationServerParam::getEdgeWrapperPtr() const
    {
        return edge_wrapper_ptr_;
    }

    RingBuffer<SynchronizationServerItem>* SynchronizationServerParam::getSynchronizationServerItemBufferPtr() const
    {
        return synchronization_server_item_buffer_ptr_;
    }
}