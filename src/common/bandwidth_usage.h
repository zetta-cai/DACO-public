/*
 * BandwidthUsage: count client-edge, cross-edge, and edge-cloud bandwidth usage (in units of bytes).
 *
 * NOTE: empty BandwidthUsage in issued requests; update BandwidthUsage for received requests; embed BandwidthUsage for issued responses; update BandwidthUsage for received responses.
 * 
 * NOTE: use uint64_t for bandwidth bytes to avoid integer overflow.
 * 
 * By Siyuan Sheng (2023.09.14).
 */

#ifndef BANDWIDTH_USAGE_H
#define BANDWIDTH_USAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/message_base.h"

namespace covered
{
    class BandwidthUsage
    {
    public:
        BandwidthUsage();
        BandwidthUsage(const uint64_t& client_edge_bandwidth_bytes, const uint64_t& cross_edge_bandwidth_bytes, const uint64_t& edge_cloud_bandwidth_bytes, const uint64_t& client_edge_msgcnt, const uint64_t& cross_edge_msgcnt, const uint64_t& edge_cloud_msgcnt, const MessageType& message_type, const uint32_t& victim_syncset_bandwidth_bytes = 0); // NOTE: message_type ONLY works for cross-edge network overhead
        ~BandwidthUsage();

        void update(const BandwidthUsage& other); // Add other into *this

        uint64_t getClientEdgeBandwidthBytes() const;
        uint64_t getCrossEdgeControlContentDiscoveryBandwidthBytes() const;
        uint64_t getCrossEdgeControlDirectoryUpdateBandwidthBytes() const;
        uint64_t getCrossEdgeControlOthersBandwidthBytes() const;
        uint64_t getCrossEdgeControlTotalBandwidthBytes() const;
        uint64_t getCrossEdgeDataBandwidthBytes() const;
        uint64_t getEdgeCloudBandwidthBytes() const;

        uint64_t getVictimSyncsetBandwidthBytes() const;

        uint64_t getClientEdgeMsgcnt() const;
        uint64_t getCrossEdgeControlContentDiscoveryMsgcnt() const;
        uint64_t getCrossEdgeControlDirectoryUpdateMsgcnt() const;
        uint64_t getCrossEdgeControlOthersMsgcnt() const;
        uint64_t getCrossEdgeControlTotalMsgcnt() const;
        uint64_t getCrossEdgeDataMsgcnt() const;
        uint64_t getEdgeCloudMsgcnt() const;

        static uint32_t getBandwidthUsagePayloadSize();
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const BandwidthUsage& operator=(const BandwidthUsage& other);
    private:
        static const std::string kClassName;

        void reset_();

        uint64_t client_edge_bandwidth_bytes_; // Must be data messages
        uint64_t cross_edge_control_content_discovery_bandwidth_bytes_; // Bandwidth cost of cross-edge control messages for content discovery
        uint64_t cross_edge_control_directory_udpate_bandwidth_bytes_; // Bandwidth cost of cross-edge control messages for directory update
        uint64_t cross_edge_control_others_bandwidth_bytes_; // Bandwidth cost of cross-edge control messages for other purposes
        uint64_t cross_edge_data_bandwidth_bytes_; // Bandwidth cost of cross-edge data messages
        uint64_t edge_cloud_bandwidth_bytes_; // Must be data messages

        uint64_t victim_syncset_bandwidth_bytes_; // Must be cross-edge (control/data)

        uint64_t client_edge_msgcnt_; // Must be data messages
        uint64_t cross_edge_control_content_discovery_msgcnt_; // Message count of cross-edge control messages for content discovery
        uint64_t cross_edge_control_directory_udpate_msgcnt_; // Message count of cross-edge control messages for directory update
        uint64_t cross_edge_control_others_msgcnt_; // Message count of cross-edge control messages for other purposes
        uint64_t cross_edge_data_msgcnt_; // Message count of cross-edge data messages
        uint64_t edge_cloud_msgcnt_; // Must be data messages
    };
}

#endif