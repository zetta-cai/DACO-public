/*
 * CLILatencyInfo: store the propagation CLI parameters.
 * 
 * By Siyuan Sheng (2024.08.01).
 */

#ifndef CLI_LATENCY_INFO_H
#define CLI_LATENCY_INFO_H

#include <string>

namespace covered
{
    class CLILatencyInfo
    {
    public:
        CLILatencyInfo();
        CLILatencyInfo(const std::string& propagation_latency_distname, const uint32_t& propagation_latency_clientedge_lbound_us, const uint32_t& propagation_latency_clientedge_avg_us, const uint32_t& propagation_latency_clientedge_rbound_us, const uint32_t& propagation_latency_crossedge_lbound_us, const uint32_t& propagation_latency_crossedge_avg_us, const uint32_t& propagation_latency_crossedge_rbound_us, const uint32_t& propagation_latency_edgecloud_lbound_us, const uint32_t& propagation_latency_edgecloud_avg_us, const uint32_t& propagation_latency_edgecloud_rbound_us);
        ~CLILatencyInfo();

        std::string getPropagationLatencyDistname() const;
        uint32_t getPropagationLatencyClientedgeLboundUs() const;
        uint32_t getPropagationLatencyClientedgeAvgUs() const;
        uint32_t getPropagationLatencyClientedgeRboundUs() const;
        uint32_t getPropagationLatencyCrossedgeLboundUs() const;
        uint32_t getPropagationLatencyCrossedgeAvgUs() const;
        uint32_t getPropagationLatencyCrossedgeRboundUs() const;
        uint32_t getPropagationLatencyEdgecloudLboundUs() const;
        uint32_t getPropagationLatencyEdgecloudAvgUs() const;
        uint32_t getPropagationLatencyEdgecloudRboundUs() const;

        CLILatencyInfo& operator=(const CLILatencyInfo& other);
    private:
        static const std::string kClassName;

        std::string propagation_latency_distname_; // Latency distribution name
        uint32_t propagation_latency_clientedge_lbound_us_; // Left bound RTT between client and edge
        uint32_t propagation_latency_clientedge_avg_us_; // Average RTT between client and edge
        uint32_t propagation_latency_clientedge_rbound_us_; // Right bound RTT between client and edge
        uint32_t propagation_latency_crossedge_lbound_us_; // Left bound RTT between edge and neighbor
        uint32_t propagation_latency_crossedge_avg_us_; // Average RTT between edge and neighbor
        uint32_t propagation_latency_crossedge_rbound_us_; // Right bound RTT between edge and neighbor
        uint32_t propagation_latency_edgecloud_lbound_us_; // Left bound RTT between edge and cloud
        uint32_t propagation_latency_edgecloud_avg_us_; // Average RTT between edge and cloud
        uint32_t propagation_latency_edgecloud_rbound_us_; // Right bound RTT between edge and cloud
    };
}

#endif