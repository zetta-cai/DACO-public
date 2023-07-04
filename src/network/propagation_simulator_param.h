/*
 * PropagationSimulatorParam: param shared by working threads of each node and the corresponding PropagationSimulator, which tracks messages to be issued after propagation latency (thread safe).
 *
 * NOTE: all PropagationItems in a PropagationSimulatorParam must have the same propagation latency, otherwise PropagationSimulator cannot simulate propagation latency for each item correctly.
 * 
 * By Siyuan Sheng (2023.07.03).
 */

#ifndef PROPAGATION_SIMULATOR_PARAM_H
#define PROPAGATION_SIMULATOR_PARAM_H

#include <mutex>
#include <string>
#include <time.h>

#include "common/node_param_base.h"
#include "concurrency/ring_buffer_impl.h"
#include "network/propagation_item.h"

namespace covered
{
    class PropagationSimulatorParam
    {
    public:
        PropagationSimulatorParam();
        PropagationSimulatorParam(const uint32_t& propagation_latency_us, NodeParamBase* node_param_ptr, const uint32_t& propagation_item_buffer_size);
        ~PropagationSimulatorParam();

        PropagationSimulatorParam& operator=(const PropagationSimulatorParam& other);
    private:
        static const std::string kClassName;

        // Const shared variables
        uint32_t propagation_latency_us_;
        NodeParamBase* node_param_ptr_;
        
        // Non-const variables shared by working threads of each ndoe and propagation simulator
        std::mutex mutex_lock_; // Ensure the atomicity of ring buffer due to multiple providers
        RingBuffer<PropagationItem>* propagation_item_buffer_ptr_;
        struct timespec prev_timespec_;
    };
}



#endif