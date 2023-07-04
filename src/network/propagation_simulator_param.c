#include "network/propagation_simulator_param.h"

#include <assert.h>

namespace covered
{
    const std::string PropagationSimulatorParam::kClassName("PropagationSimulatorParam");

    PropagationSimulatorParam::PropagationSimulatorParam() : propagation_latency_us_(0), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), prev_timespec_()
    {
        propagation_item_buffer_ptr_ = NULL;
        node_param_ptr_ = NULL;
    }

    PropagationSimulatorParam::PropagationSimulatorParam(const uint32_t& propagation_latency_us, NodeParamBase* node_param_ptr, const uint32_t& propagation_item_buffer_size) : propagation_latency_us_(propagation_latency_us), node_param_ptr_(node_param_ptr), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), prev_timespec_()
    {
        propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), propagation_item_buffer_size);
        assert(propagation_item_buffer_ptr_ != NULL);

        assert(node_param_ptr_ != NULL);
    }

    PropagationSimulatorParam::~PropagationSimulatorParam()
    {
        // NOTE: no need to release node_param_ptr_, which will be released outside PropagationSimulatorParam (e.g., in simulator)

        assert(propagation_item_buffer_ptr_ != NULL);
        delete propagation_item_buffer_ptr_;
        propagation_item_buffer_ptr_ = NULL;
    }

    uint32_t PropagationSimulatorParam::getPropagationLatencyUs() const
    {
        // No need to acquire a lock due to const shared variable
        return propagation_latency_us_;
    }
    
    NodeParamBase* PropagationSimulatorParam::getNodeParamPtr() const
    {
        // No need to acquire a lock due to const shared variable
        return node_param_ptr_;
    }

    bool PropagationSimulatorParam::pop(PropagationItem& element)
    {
        // Acquire a write lock
        std::string context_name = "PropagationSimulatorParam::pop()";
        rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        bool is_successful = propagation_item_buffer_ptr_->pop(element);

        // Release a write lock
        rwlock_for_propagation_item_buffer_.unlock(context_name);

        return is_successful;
    }

    PropagationSimulatorParam& PropagationSimulatorParam::operator=(const PropagationSimulatorParam& other)
    {
        propagation_latency_us_ = other.propagation_latency_us_;

        node_param_ptr_ = other.node_param_ptr_;
        assert(node_param_ptr_ != NULL);
        
        // Not copy mutex_lock_

        // Deep copy propagation_item_buffer_ptr_
        if (propagation_item_buffer_ptr_ != NULL)
        {
            delete propagation_item_buffer_ptr_;
            propagation_item_buffer_ptr_ = NULL;
        }
        if (other.propagation_item_buffer_ptr_ != NULL)
        {
            propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), other.propagation_item_buffer_ptr_->getBufferSize());
            assert(propagation_item_buffer_ptr_ != NULL);
            
            *propagation_item_buffer_ptr_ = *other.propagation_item_buffer_ptr_; // deep copy
        }

        prev_timespec_ = other.prev_timespec_;
        
        return *this;
    }
}