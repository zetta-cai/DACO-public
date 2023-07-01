/*
 * PropagationSimulator: simulate client-edge-cloud propagation latency.
 * 
 * By Siyuan Sheng (2023.05.26).
 */

#ifndef PROPAGATION_SIMULATOR_H
#define PROPAGATION_SIMULATOR_H

#define DEBUG_PROPAGATION_SIMULATOR

#include <string>

namespace covered
{
    class PropagationSimulator
    {
    public:
        static void propagateFromClientToEdge();
        static void propagateFromEdgeToClient();
        static void propagateFromEdgeToNeighbor(); // Neighbor edge node: beacon or target
        static void propagateFromNeighborToEdge(); // Neighbor edge node: beacon or target
        static void propagateFromEdgeToCloud();
        static void propagateFromCloudToEdge();
    private:
        static std::string kClassName;

        static void propagate_(const uint32_t& latency_us);
    };
}

#endif