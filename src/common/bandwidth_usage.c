#include "common/bandwidth_usage.h"

//#include <arpa/inet.h> // htonl ntohl
#include <assert.h>

#include "message/message_base.h"

namespace covered
{
    BandwidthUsage::BandwidthUsage()
    {
        reset_();
    }

    BandwidthUsage::BandwidthUsage(const uint64_t& client_edge_bandwidth_bytes, const uint64_t& cross_edge_bandwidth_bytes, const uint64_t& edge_cloud_bandwidth_bytes, const uint64_t& client_edge_msgcnt, const uint64_t& cross_edge_msgcnt, const uint64_t& edge_cloud_msgcnt, const MessageType& message_type, const uint32_t& victim_syncset_bandwidth_bytes)
    {
        if (client_edge_bandwidth_bytes == 0 || client_edge_msgcnt == 0)
        {
            assert(client_edge_bandwidth_bytes == 0);
            assert(client_edge_msgcnt == 0);
        }
        if (cross_edge_bandwidth_bytes == 0 || cross_edge_msgcnt == 0)
        {
            assert(cross_edge_bandwidth_bytes == 0);
            assert(cross_edge_msgcnt == 0);
        }
        if (edge_cloud_bandwidth_bytes == 0 || edge_cloud_msgcnt == 0)
        {
            assert(edge_cloud_bandwidth_bytes == 0);
            assert(edge_cloud_msgcnt == 0);
        }

        reset_();

        // Update bandwidth usage unrelated with message type
        client_edge_bandwidth_bytes_ = client_edge_bandwidth_bytes;
        edge_cloud_bandwidth_bytes_ = edge_cloud_bandwidth_bytes;
        victim_syncset_bandwidth_bytes_ = victim_syncset_bandwidth_bytes;
        client_edge_msgcnt_ = client_edge_msgcnt;
        edge_cloud_msgcnt_ = edge_cloud_msgcnt;

        // NOTE: it is okay to set is_data_message as true for local/global data messages, as cross_edge_bandwidth_bytes equals 0, which does not affect cross-edge data network statistics
        const bool is_data_message = MessageBase::isDataRequest(message_type) || MessageBase::isDataResponse(message_type);
        const bool is_content_discovery_message = MessageBase::isDirectoryLookupMessage(message_type);
        const bool is_directory_update_message = MessageBase::isDirectoryUpdateMessage(message_type);
        
        if (is_data_message)
        {
            cross_edge_data_bandwidth_bytes_ = cross_edge_bandwidth_bytes;
        }
        else
        {
            if (is_content_discovery_message)
            {
                cross_edge_control_content_discovery_bandwidth_bytes_ = cross_edge_bandwidth_bytes;
            }
            else if (is_directory_update_message)
            {
                cross_edge_control_directory_udpate_bandwidth_bytes_ = cross_edge_bandwidth_bytes;
            }
            else
            {
                cross_edge_control_others_bandwidth_bytes_ = cross_edge_bandwidth_bytes;
            }
        }
        
        if (is_data_message)
        {
            cross_edge_data_msgcnt_ = cross_edge_msgcnt;
        }
        else
        {
            if (is_content_discovery_message)
            {
                cross_edge_control_content_discovery_msgcnt_ = cross_edge_msgcnt;
            }
            else if (is_directory_update_message)
            {
                cross_edge_control_directory_udpate_msgcnt_ = cross_edge_msgcnt;
            }
            else
            {
                cross_edge_control_others_msgcnt_ = cross_edge_msgcnt;
            }
        }
    }

    BandwidthUsage::BandwidthUsage(const BandwidthUsage& other)
    {
        client_edge_bandwidth_bytes_ = other.client_edge_bandwidth_bytes_;
        cross_edge_control_content_discovery_bandwidth_bytes_ = other.cross_edge_control_content_discovery_bandwidth_bytes_;
        cross_edge_control_directory_udpate_bandwidth_bytes_ = other.cross_edge_control_directory_udpate_bandwidth_bytes_;
        cross_edge_control_others_bandwidth_bytes_ = other.cross_edge_control_others_bandwidth_bytes_;
        cross_edge_data_bandwidth_bytes_ = other.cross_edge_data_bandwidth_bytes_;
        edge_cloud_bandwidth_bytes_ = other.edge_cloud_bandwidth_bytes_;

        victim_syncset_bandwidth_bytes_ = other.victim_syncset_bandwidth_bytes_;

        client_edge_msgcnt_ = other.client_edge_msgcnt_;
        cross_edge_control_content_discovery_msgcnt_ = other.cross_edge_control_content_discovery_msgcnt_;
        cross_edge_control_directory_udpate_msgcnt_ = other.cross_edge_control_directory_udpate_msgcnt_;
        cross_edge_control_others_msgcnt_ = other.cross_edge_control_others_msgcnt_;
        cross_edge_data_msgcnt_ = other.cross_edge_data_msgcnt_;
        edge_cloud_msgcnt_ = other.edge_cloud_msgcnt_;
    }

    BandwidthUsage::~BandwidthUsage() {}

    void BandwidthUsage::update(const BandwidthUsage& other)
    {
        client_edge_bandwidth_bytes_ += other.client_edge_bandwidth_bytes_;
        cross_edge_data_bandwidth_bytes_ += other.cross_edge_data_bandwidth_bytes_;
        cross_edge_control_content_discovery_bandwidth_bytes_ += other.cross_edge_control_content_discovery_bandwidth_bytes_;
        cross_edge_control_directory_udpate_bandwidth_bytes_ += other.cross_edge_control_directory_udpate_bandwidth_bytes_;
        cross_edge_control_others_bandwidth_bytes_ += other.cross_edge_control_others_bandwidth_bytes_;
        edge_cloud_bandwidth_bytes_ += other.edge_cloud_bandwidth_bytes_;

        victim_syncset_bandwidth_bytes_ += other.victim_syncset_bandwidth_bytes_;

        client_edge_msgcnt_ += other.client_edge_msgcnt_;
        cross_edge_data_msgcnt_ += other.cross_edge_data_msgcnt_;
        cross_edge_control_content_discovery_msgcnt_ += other.cross_edge_control_content_discovery_msgcnt_;
        cross_edge_control_directory_udpate_msgcnt_ += other.cross_edge_control_directory_udpate_msgcnt_;
        cross_edge_control_others_msgcnt_ += other.cross_edge_control_others_msgcnt_;
        edge_cloud_msgcnt_ += other.edge_cloud_msgcnt_;

        return;
    }

    // Fine-grained update for single-node simulation

    void BandwidthUsage::updateClientEdgeBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        client_edge_bandwidth_bytes_ += bandwidth_bytes;
        client_edge_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateCrossEdgeControlContentDiscoveryBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        cross_edge_control_content_discovery_bandwidth_bytes_ += bandwidth_bytes;
        cross_edge_control_content_discovery_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateCrossEdgeControlDirectoryUpdateBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        cross_edge_control_directory_udpate_bandwidth_bytes_ += bandwidth_bytes;
        cross_edge_control_directory_udpate_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateCrossEdgeControlOthersBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        cross_edge_control_others_bandwidth_bytes_ += bandwidth_bytes;
        cross_edge_control_others_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateCrossEdgeDataBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        cross_edge_data_bandwidth_bytes_ += bandwidth_bytes;
        cross_edge_data_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateEdgeCloudBandwidthAndMsgcnt(const uint64_t& bandwidth_bytes, const uint64_t& msgcnt)
    {
        edge_cloud_bandwidth_bytes_ += bandwidth_bytes;
        edge_cloud_msgcnt_ += msgcnt;
        return;
    }

    void BandwidthUsage::updateVictimSyncsetBandwidth(const uint32_t& bandwidth_bytes)
    {
        victim_syncset_bandwidth_bytes_ += bandwidth_bytes;
        return;
    }

    uint64_t BandwidthUsage::getClientEdgeBandwidthBytes() const
    {
        return client_edge_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlContentDiscoveryBandwidthBytes() const
    {
        return cross_edge_control_content_discovery_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlDirectoryUpdateBandwidthBytes() const
    {
        return cross_edge_control_directory_udpate_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlOthersBandwidthBytes() const
    {
        return cross_edge_control_others_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlTotalBandwidthBytes() const
    {
        return cross_edge_control_content_discovery_bandwidth_bytes_ + cross_edge_control_directory_udpate_bandwidth_bytes_ + cross_edge_control_others_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getCrossEdgeDataBandwidthBytes() const
    {
        return cross_edge_data_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getEdgeCloudBandwidthBytes() const
    {
        return edge_cloud_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getVictimSyncsetBandwidthBytes() const
    {
        return victim_syncset_bandwidth_bytes_;
    }

    uint64_t BandwidthUsage::getClientEdgeMsgcnt() const
    {
        return client_edge_msgcnt_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlContentDiscoveryMsgcnt() const
    {
        return cross_edge_control_content_discovery_msgcnt_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlDirectoryUpdateMsgcnt() const
    {
        return cross_edge_control_directory_udpate_msgcnt_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlOthersMsgcnt() const
    {
        return cross_edge_control_others_msgcnt_;
    }

    uint64_t BandwidthUsage::getCrossEdgeControlTotalMsgcnt() const
    {
        return cross_edge_control_content_discovery_msgcnt_ + cross_edge_control_directory_udpate_msgcnt_ + cross_edge_control_others_msgcnt_;
    }

    uint64_t BandwidthUsage::getCrossEdgeDataMsgcnt() const
    {
        return cross_edge_data_msgcnt_;
    }

    uint64_t BandwidthUsage::getEdgeCloudMsgcnt() const
    {
        return edge_cloud_msgcnt_;
    }

    uint32_t BandwidthUsage::getBandwidthUsagePayloadSize()
    {
        // client-edge bandwidth usage + cross-edge bandwidth usage (control * 3 and data + victim syncset) + edge-cloud bandwidth usage
        uint32_t bandwidth_bytes_payload = sizeof(uint64_t) + sizeof(uint64_t) * 5 + sizeof(uint64_t);

        // client-edge message count + cross-edge message count (control * 3 and data) + edge-cloud message count
        uint32_t msgcnt_payload = sizeof(uint64_t) + sizeof(uint64_t) * 4 + sizeof(uint64_t);

        return bandwidth_bytes_payload + msgcnt_payload;
    }

    /*uint32_t BandwidthUsage::serialize(DynamicArray& msg_payload, const uint32_t& position) const
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
    }*/

    uint32_t BandwidthUsage::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;

        msg_payload.deserialize(size, (const char *)&client_edge_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char *)&cross_edge_control_content_discovery_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char *)&cross_edge_control_directory_udpate_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char *)&cross_edge_control_others_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char *)&cross_edge_data_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char *)&edge_cloud_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&victim_syncset_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        
        msg_payload.deserialize(size, (const char*)&client_edge_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cross_edge_control_content_discovery_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cross_edge_control_directory_udpate_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cross_edge_control_others_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cross_edge_data_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&edge_cloud_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    uint32_t BandwidthUsage::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;

        msg_payload.serialize(size, (char *)&client_edge_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cross_edge_control_content_discovery_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cross_edge_control_directory_udpate_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cross_edge_control_others_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cross_edge_data_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&edge_cloud_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&victim_syncset_bandwidth_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);

        msg_payload.serialize(size, (char*)&client_edge_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&cross_edge_control_content_discovery_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&cross_edge_control_directory_udpate_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&cross_edge_control_others_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&cross_edge_data_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char*)&edge_cloud_msgcnt_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    const BandwidthUsage& BandwidthUsage::operator=(const BandwidthUsage& other)
    {
        client_edge_bandwidth_bytes_ = other.client_edge_bandwidth_bytes_;
        cross_edge_control_content_discovery_bandwidth_bytes_ = other.cross_edge_control_content_discovery_bandwidth_bytes_;
        cross_edge_control_directory_udpate_bandwidth_bytes_ = other.cross_edge_control_directory_udpate_bandwidth_bytes_;
        cross_edge_control_others_bandwidth_bytes_ = other.cross_edge_control_others_bandwidth_bytes_;
        cross_edge_data_bandwidth_bytes_ = other.cross_edge_data_bandwidth_bytes_;
        edge_cloud_bandwidth_bytes_ = other.edge_cloud_bandwidth_bytes_;

        victim_syncset_bandwidth_bytes_ = other.victim_syncset_bandwidth_bytes_;

        client_edge_msgcnt_ = other.client_edge_msgcnt_;
        cross_edge_control_content_discovery_msgcnt_ = other.cross_edge_control_content_discovery_msgcnt_;
        cross_edge_control_directory_udpate_msgcnt_ = other.cross_edge_control_directory_udpate_msgcnt_;
        cross_edge_control_others_msgcnt_ = other.cross_edge_control_others_msgcnt_;
        cross_edge_data_msgcnt_ = other.cross_edge_data_msgcnt_;
        edge_cloud_msgcnt_ = other.edge_cloud_msgcnt_;

        return *this;
    }

    void BandwidthUsage::reset_()
    {
        client_edge_bandwidth_bytes_ = 0;
        cross_edge_control_content_discovery_bandwidth_bytes_ = 0;
        cross_edge_control_directory_udpate_bandwidth_bytes_ = 0;
        cross_edge_control_others_bandwidth_bytes_ = 0;
        cross_edge_data_bandwidth_bytes_ = 0;
        edge_cloud_bandwidth_bytes_ = 0;
        
        victim_syncset_bandwidth_bytes_ = 0;

        client_edge_msgcnt_ = 0;
        cross_edge_control_content_discovery_msgcnt_ = 0;
        cross_edge_control_directory_udpate_msgcnt_ = 0;
        cross_edge_control_others_msgcnt_ = 0;
        cross_edge_data_msgcnt_ = 0;
        edge_cloud_msgcnt_ = 0;

        return;
    }
}