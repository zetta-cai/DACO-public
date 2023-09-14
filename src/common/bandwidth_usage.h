/*
 * BandwidthUsage: count client-edge, cross-edge, and edge-cloud bandwidth usage (in units of bytes).
 * 
 * By Siyuan Sheng (2023.09.14).
 */

#ifndef BANDWIDTH_USAGE_H
#define BANDWIDTH_USAGE_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class BandwidthUsage
    {
    public:
        BandwidthUsage();
        BandwidthUsage(const uint32_t& client_edge_bandwidth_bytes, const uint32_t& cross_edge_bandwidth_bytes, const uint32_t& edge_cloud_bandwidth_bytes);
        ~BandwidthUsage();

        uint32_t getClientEdgeBandwidthBytes() const;
        uint32_t getCrossEdgeBandwidthBytes() const;
        uint32_t getEdgeCloudBandwidthBytes() const;

        uint32_t getBandwidthUsagePayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const BandwidthUsage& operator=(const BandwidthUsage& other);
    private:
        static const std::string kClassName;

        uint32_t client_edge_bandwidth_bytes_;
        uint32_t cross_edge_bandwidth_bytes_;
        uint32_t edge_cloud_bandwidth_bytes_;
    };
}

#endif