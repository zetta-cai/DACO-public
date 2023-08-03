/*
 * PropagationParam: store CLI parameters for propagation dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef PROPAGATION_PARAM_H
#define PROPAGATION_PARAM_H

#include <string>

namespace covered
{
    class PropagationParam
    {
    public:
        static void setParameters(const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us);

        static uint32_t getPropagationLatencyClientedgeUs();
        static uint32_t getPropagationLatencyCrossedgeUs();
        static uint32_t getPropagationLatencyEdgecloudUs();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid_();

        static uint32_t propagation_latency_clientedge_us_; // 1/2 RTT between client and edge (bidirectional link)
        static uint32_t propagation_latency_crossedge_us_; // 1/2 RTT between edge and neighbor (bidirectional link)
        static uint32_t propagation_latency_edgecloud_us_; // 1/2 RTT between edge and cloud (bidirectional link)

        static bool is_valid_;
    };
}

#endif