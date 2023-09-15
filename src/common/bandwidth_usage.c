#include "common/bandwidth_usage.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    BandwidthUsage::BandwidthUsage()
    {
        client_edge_bandwidth_bytes_ = 0;
        cross_edge_bandwidth_bytes_ = 0;
        edge_cloud_bandwidth_bytes_ = 0;
    }

    BandwidthUsage::BandwidthUsage(const uint32_t& client_edge_bandwidth_bytes, const uint32_t& cross_edge_bandwidth_bytes, const uint32_t& edge_cloud_bandwidth_bytes)
    {
        client_edge_bandwidth_bytes_ = client_edge_bandwidth_bytes;
        cross_edge_bandwidth_bytes_ = cross_edge_bandwidth_bytes;
        edge_cloud_bandwidth_bytes_ = edge_cloud_bandwidth_bytes;
    }

    BandwidthUsage::~BandwidthUsage() {}

    void BandwidthUsage::update(const BandwidthUsage& other)
    {
        client_edge_bandwidth_bytes_ += other.client_edge_bandwidth_bytes_;
        cross_edge_bandwidth_bytes_ += other.cross_edge_bandwidth_bytes_;
        edge_cloud_bandwidth_bytes_ += other.edge_cloud_bandwidth_bytes_;
        return;
    }

    uint32_t BandwidthUsage::getClientEdgeBandwidthBytes() const
    {
        return client_edge_bandwidth_bytes_;
    }

    uint32_t BandwidthUsage::getCrossEdgeBandwidthBytes() const
    {
        return cross_edge_bandwidth_bytes_;
    }

    uint32_t BandwidthUsage::getEdgeCloudBandwidthBytes() const
    {
        return edge_cloud_bandwidth_bytes_;
    }

    uint32_t BandwidthUsage::getBandwidthUsagePayloadSize() const
    {
        // client-edge bandwidth usage + cross-edge bandwidth usage + edge-cloud bandwidth usage
        return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
    }

    uint32_t BandwidthUsage::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_client_edge_bandwidth_bytes = htonl(client_edge_bandwidth_bytes_);
        msg_payload.deserialize(size, (const char *)&bigendian_client_edge_bandwidth_bytes, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_cross_edge_bandwidth_bytes = htonl(cross_edge_bandwidth_bytes_);
        msg_payload.deserialize(size, (const char *)&bigendian_cross_edge_bandwidth_bytes, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_edge_cloud_bandwidth_bytes = htonl(edge_cloud_bandwidth_bytes_);
        msg_payload.deserialize(size, (const char *)&bigendian_edge_cloud_bandwidth_bytes, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t BandwidthUsage::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t bigendian_client_edge_bandwidth_bytes;
        msg_payload.serialize(size, (char *)&bigendian_client_edge_bandwidth_bytes, sizeof(uint32_t));
        client_edge_bandwidth_bytes_ = ntohl(bigendian_client_edge_bandwidth_bytes);
        size += sizeof(uint32_t);
        uint32_t bigendian_cross_edge_bandwidth_bytes;
        msg_payload.serialize(size, (char *)&bigendian_cross_edge_bandwidth_bytes, sizeof(uint32_t));
        cross_edge_bandwidth_bytes_ = ntohl(bigendian_cross_edge_bandwidth_bytes);
        size += sizeof(uint32_t);
        uint32_t bigendian_edge_cloud_bandwidth_bytes;
        msg_payload.serialize(size, (char *)&bigendian_edge_cloud_bandwidth_bytes, sizeof(uint32_t));
        edge_cloud_bandwidth_bytes_ = ntohl(bigendian_edge_cloud_bandwidth_bytes);
        size += sizeof(uint32_t);
        return size - position;
    }

    const BandwidthUsage& BandwidthUsage::operator=(const BandwidthUsage& other)
    {
        client_edge_bandwidth_bytes_ = other.client_edge_bandwidth_bytes_;
        cross_edge_bandwidth_bytes_ = other.cross_edge_bandwidth_bytes_;
        edge_cloud_bandwidth_bytes_ = other.edge_cloud_bandwidth_bytes_;
        return *this;
    }
}