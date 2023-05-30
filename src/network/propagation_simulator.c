#include "network/propagation_simulator.h"

#include <unistd.h> // usleep

#include "common/param.h"

namespace covered
{
    std::string PropagationSimulator::kClassName("PropagationSimulator");

    void PropagationSimulator::propagateFromClientToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToClient()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToNeighbor()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromNeighborToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToCloud()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromCloudToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        propagate_(latency_us);
        return;
    }
        
    void PropagationSimulator::propagate_(const uint32_t& latency_us)
    {
        usleep(latency_us);
        return;
    }
}