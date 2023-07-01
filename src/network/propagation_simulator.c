#include "network/propagation_simulator.h"

#include <unistd.h> // usleep

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    std::string PropagationSimulator::kClassName("PropagationSimulator");

    void PropagationSimulator::propagateFromClientToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromClientToEdge;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToClient()
    {
        uint32_t latency_us = Param::getPropagationLatencyClientedge();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromEdgeToClient;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToNeighbor()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromEdgeToNeighbor;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromNeighborToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyCrossedge();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromNeighborToEdge;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromEdgeToCloud()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromEdgeToCloud;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
    
    void PropagationSimulator::propagateFromCloudToEdge()
    {
        uint32_t latency_us = Param::getPropagationLatencyEdgecloud();
        #ifdef DEBUG_PROPAGATION_SIMULATOR
        Util::dumpVariablesForDebug(kClassName, 3, "propagateFromCloudToEdge;", "latency us:", Util::toString(latency_us).c_str());
        #endif
        propagate_(latency_us);
        return;
    }
        
    void PropagationSimulator::propagate_(const uint32_t& latency_us)
    {
        usleep(latency_us);
        return;
    }
}