#include "network/propagation_simulator_param.h"

#include <assert.h>

namespace covered
{
    const std::string PropagationSimulatorParam::kClassName("PropagationSimulatorParam");

    PropagationSimulatorParam::PropagationSimulatorParam() : propagation_latency_us_(0), mutex_lock_(), prev_timespec_()
    {
        propagation_item_buffer_ptr_ = NULL;
    }

    PropagationSimulatorParam::PropagationSimulatorParam(const uint32_t& propagation_latency_us, const uint32_t& propagation_item_buffer_size) : propagation_latency_us_(propagation_latency_us), mutex_lock_(), prev_timespec_()
    {
        propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), propagation_item_buffer_size);
        assert(propagation_item_buffer_ptr_ != NULL);
    }

    PropagationSimulatorParam::~PropagationSimulatorParam()
    {
        assert(propagation_item_buffer_ptr_ != NULL);
        delete propagation_item_buffer_ptr_;
        propagation_item_buffer_ptr_ = NULL;
    }

    PropagationSimulatorParam& PropagationSimulatorParam::operator=(const PropagationSimulatorParam& other)
    {
        propagation_latency_us_ = other.propagation_latency_us_;
        
        // Not copy mutex_lock_

        // Deep copy propagation_item_buffer_ptr_
        assert(propagation_item_buffer_ptr_ == NULL); // Must copy a temporary PropagationSimulatorParam to a default PropagationSimulatorParam
        propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), other.propagation_item_buffer_ptr_->getBufferSize());
        assert(propagation_item_buffer_ptr_ != NULL);
        *propagation_item_buffer_ptr_ = *other.propagation_item_buffer_ptr_; // deep copy

        prev_timespec_ = other.prev_timespec_;
        
        return *this;
    }
}