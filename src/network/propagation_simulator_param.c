#include "network/propagation_simulator_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string PropagationSimulatorParam::kClassName("PropagationSimulatorParam");

    PropagationSimulatorParam::PropagationSimulatorParam() : propagation_latency_us_(0), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), is_first_item_(true), prev_timespec_()
    {
        propagation_item_buffer_ptr_ = NULL;
        instance_name_ = "";
    }

    PropagationSimulatorParam::PropagationSimulatorParam(NodeWrapperBase* node_wrapper_ptr, const uint32_t& propagation_latency_us, const uint32_t& propagation_item_buffer_size) : node_wrapper_ptr_(node_wrapper_ptr), propagation_latency_us_(propagation_latency_us), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), is_first_item_(true), prev_timespec_()
    {
        assert(node_wrapper_ptr != NULL);

        // Differential propagation simulator parameter of different nodes
        std::ostringstream oss;
        oss << kClassName << " " << node_wrapper_ptr->getNodeRoleIdxStr();
        instance_name_ = oss.str();

        propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), propagation_item_buffer_size);
        assert(propagation_item_buffer_ptr_ != NULL);
    }

    PropagationSimulatorParam::~PropagationSimulatorParam()
    {
        assert(propagation_item_buffer_ptr_ != NULL);
        delete propagation_item_buffer_ptr_;
        propagation_item_buffer_ptr_ = NULL;
    }

    const NodeWrapperBase* PropagationSimulatorParam::getNodeWrapperPtr() const
    {
        // No need to acquire a lock due to const shared variable
        assert(node_wrapper_ptr_ != NULL);
        return node_wrapper_ptr_;
    }

    uint32_t PropagationSimulatorParam::getPropagationLatencyUs() const
    {
        // No need to acquire a lock due to const shared variable
        return propagation_latency_us_;
    }

    bool PropagationSimulatorParam::push(MessageBase* message_ptr, const NetworkAddr& dst_addr)
    {
        assert(message_ptr != NULL);
        assert(dst_addr.isValidAddr());

        // Acquire a write lock
        std::string context_name = "PropagationSimulatorParam::push()";
        rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        const bool skip_propagation_latency = message_ptr->isSkipPropagationLatency();
        uint32_t sleep_us = 0;
        if (skip_propagation_latency)
        {
            // skip_propagation_latency = true means the message is enabled with warmup speedup under warmup phase
            sleep_us = 0;
        }
        else
        {
            // Calculate sleep interval
            struct timespec cur_timespec = Util::getCurrentTimespec();
            if (is_first_item_)
            {
                sleep_us = propagation_latency_us_;

                prev_timespec_ = cur_timespec;
                is_first_item_ = false;
            }
            else
            {
                sleep_us = Util::getDeltaTimeUs(cur_timespec, prev_timespec_);
                if (sleep_us > propagation_latency_us_)
                {
                    sleep_us = propagation_latency_us_;
                }

                prev_timespec_ = cur_timespec;
            }
        }
        assert(sleep_us <= propagation_latency_us_);

        // Push propagation item into ring buffer
        PropagationItem propagation_item(message_ptr, dst_addr, sleep_us);
        bool is_successful = propagation_item_buffer_ptr_->push(propagation_item);

        #ifdef DEBUG_PROPAGATION_SIMULATOR_PARAM
        //std::vector<PropagationItem> tmp_propagation_items = propagation_item_buffer_ptr_->getElementsForDebug();
        std::ostringstream oss;
        oss << "push to sleep " << sleep_us << " us to simulate a propagation latency of " << propagation_latency_us_ << " us; keystr: " << MessageBase::getKeyFromMessage(message_ptr).getKeystr() << "; dstadrr: " << dst_addr.toString() << "; srcaddr: " << message_ptr->getSourceAddr().toString() << "; ";
        //for (uint32_t i = 0; i < tmp_propagation_items.size(); i++)
        //{
        //    oss << "tmp_propagation_items[" << i << "] sleep_us: " << tmp_propagation_items[i].getSleepUs() << "; ";
        //}
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        // Release a write lock
        rwlock_for_propagation_item_buffer_.unlock(context_name);

        return is_successful;
    }

    bool PropagationSimulatorParam::pop(PropagationItem& element)
    {
        // NOTE: NO need to acquire a write lock, as there will be ONLY one reader (i.e., the propagation simulator)

        // Acquire a write lock
        //std::string context_name = "PropagationSimulatorParam::pop()";
        //rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        bool is_successful = propagation_item_buffer_ptr_->pop(element);

        // Release a write lock
        //rwlock_for_propagation_item_buffer_.unlock(context_name);

        return is_successful;
    }

    const PropagationSimulatorParam& PropagationSimulatorParam::operator=(const PropagationSimulatorParam& other)
    {
        node_wrapper_ptr_ = other.node_wrapper_ptr_;
        propagation_latency_us_ = other.propagation_latency_us_;

        instance_name_ = other.instance_name_;
        
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

        is_first_item_ = other.is_first_item_;
        prev_timespec_ = other.prev_timespec_;
        
        return *this;
    }
}