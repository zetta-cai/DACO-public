#include "network/propagation_simulator.h"

#include <unistd.h> // usleep

#include "common/param.h"

namespace covered
{
    std::string ProgatationSimulator::kClassName("ProgatationSimulator");

    void ProgatationSimulator::propagateFromClientToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        propagate_(latency_us);
        return;
    }
    
    void ProgatationSimulator::propagateFromEdgeToClient()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        propagate_(latency_us);
        return;
    }
    
    void ProgatationSimulator::propagateFromEdgeToNeighbor()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        propagate_(latency_us);
        return;
    }
    
    void ProgatationSimulator::propagateFromNeighborToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        propagate_(latency_us);
        return;
    }
    
    void ProgatationSimulator::propagateFromEdgeToCloud()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        propagate_(latency_us);
        return;
    }
    
    void ProgatationSimulator::propagateFromCloudToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        propagate_(latency_us);
        return;
    }
        
    void ProgatationSimulator::propagate_(const uint32_t& latency_us)
    {
        usleep(latency_us);
        return;
    }
}