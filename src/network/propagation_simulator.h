/*
 * PropagationSimulator: simulate a given propagation latency before sending each message in client/edge/cloud node.
 *
 * NOTE: if a node has various propagation latencies to different destinations, it has launched multiple propagation simulators each for a specific propagation latency.
 * 
 * By Siyuan Sheng (2023.05.26).
 */

#ifndef PROPAGATION_SIMULATOR_H
#define PROPAGATION_SIMULATOR_H

#define DEBUG_PROPAGATION_SIMULATOR

#include <string>

#include "network/propagation_simulator_param.h"

namespace covered
{
    class PropagationSimulator
    {
    public:
        PropagationSimulator(PropagationSimulatorParam* propagation_simulator_param_ptr);
        ~PropagationSimulator();
    private:
        static std::string kClassName;

        void propagate_(const uint32_t& latency_us);

        PropagationSimulatorParam* propagation_simulator_param_ptr_;
    };
}

#endif