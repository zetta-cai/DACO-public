/*
 * PropagationSimulatorParam: param shared by working threads of each node and the corresponding PropagationSimulator, which tracks messages to be issued after propagation latency (thread safe).
 *
 * NOTE: all PropagationItems in a PropagationSimulatorParam must have the same propagation latency, otherwise PropagationSimulator cannot simulate propagation latency for each item correctly.
 * 
 * By Siyuan Sheng (2023.07.03).
 */

#ifndef PROPAGATION_SIMULATOR_PARAM_H
#define PROPAGATION_SIMULATOR_PARAM_H

//#define DEBUG_PROPAGATION_SIMULATOR_PARAM

#include <string>
#include <time.h>

#include "concurrency/ring_buffer_impl.h"
#include "concurrency/rwlock.h"
#include "common/node_wrapper_base.h"
#include "network/propagation_item.h"

namespace covered
{
    class PropagationSimulatorParam
    {
    public:
        PropagationSimulatorParam();
        PropagationSimulatorParam(NodeWrapperBase* node_wrapper_ptr, const uint32_t& propagation_latency_us, const uint32_t& propagation_item_buffer_size);
        ~PropagationSimulatorParam();

        const NodeWrapperBase* getNodeWrapperPtr() const;
        uint32_t getPropagationLatencyUs() const;

        bool push(MessageBase* message_ptr, const NetworkAddr& dst_addr);
        bool pop(PropagationItem& element); // Only invoked by PropagationSimulator

        const PropagationSimulatorParam& operator=(const PropagationSimulatorParam& other);
    private:
        static const std::string kClassName;

        // Const shared variables
        NodeWrapperBase* node_wrapper_ptr_;
        uint32_t propagation_latency_us_;
        std::string instance_name_;

        Rwlock rwlock_for_propagation_item_buffer_; // Ensure the atomicity of all non-const variables due to multiple providers (all subthreads of a client/edge/cloud node)
        
        // Non-const variables shared by working threads of each ndoe and propagation simulator
        // NOTE: there will be multiple writers (processing threads of each componenet) yet a single reader (the corresponding propagation simulator) -> NOT need to acquire lock for pop()
        RingBuffer<PropagationItem>* propagation_item_buffer_ptr_;
        bool is_first_item_;
        struct timespec prev_timespec_;
    };
}

#endif