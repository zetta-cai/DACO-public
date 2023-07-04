/*
 * PropagationSimulator: simulate a given propagation latency before sending each message in client/edge/cloud node.
 *
 * NOTE: if a node has various propagation latencies to different destinations, it has launched multiple propagation simulators each for a specific propagation latency.
 * 
 * By Siyuan Sheng (2023.07.04).
 */

#ifndef PROPAGATION_SIMULATOR_H
#define PROPAGATION_SIMULATOR_H

#define DEBUG_PROPAGATION_SIMULATOR

#include <string>

#include "network/propagation_simulator_param.h"
#include "network/udp_msg_socket_client.h"

namespace covered
{
    class PropagationSimulator
    {
    public:
        static void* launchPropagationSimulator(void* propagation_simulator_param_ptr);

        PropagationSimulator(PropagationSimulatorParam* propagation_simulator_param_ptr);
        ~PropagationSimulator();

        void start();
    private:
        static std::string kClassName;

        void propagate_(const uint32_t& sleep_us);

        void checkPointers_() const;

        // Const individual variable
        std::string instance_name_;

        // Non-const individual variable
        UdpMsgSocketClient* propagation_simulator_socket_client_ptr_;

        // Non-const variable shared by working threads of each node and propagation simulator
        PropagationSimulatorParam* propagation_simulator_param_ptr_; // thread safe
    };
}

#endif