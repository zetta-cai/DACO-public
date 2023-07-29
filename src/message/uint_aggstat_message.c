#include "message/uint_aggstat_message.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string UintAggstatMessage::kClassName("UintAggstatMessage");

    UintAggstatMessage::UintAggstatMessage(const uint32_t& unsigned_integer, const AggregatedStatisticsBase& aggregated_statistics, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        unsigned_integer_ = unsigned_integer;
        aggregated_statistics_ = aggregated_statistics;
    }

    UintAggstatMessage::UintAggstatMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    UintAggstatMessage::~UintAggstatMessage() {}

    uint32_t UintAggstatMessage::getUnsignedInteger_() const
    {
        checkIsValid_();
        return unsigned_integer_;
    }

    AggregatedStatisticsBase UintAggstatMessage::getAggregatedStatistics() const
    {
        checkIsValid_();
        return aggregated_statistics_;
    }

    uint32_t UintAggstatMessage::getMsgPayloadSizeInternal_() const
    {
        // unsigned integer + aggregated statistics
        return sizeof(uint32_t) + aggregated_statistics_.getAggregatedStatisticsIOSize();
    }

    uint32_t UintAggstatMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_unsigned_integer = htonl(unsigned_integer_);
        msg_payload.deserialize(size, (const char*)&bigendian_unsigned_integer, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t aggregated_statistics_serialize_size = aggregated_statistics_.serialize(msg_payload, size);
        size += aggregated_statistics_serialize_size;
        return size - position;
    }

    uint32_t UintAggstatMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&unsigned_integer_, sizeof(uint32_t));
        unsigned_integer_ = ntohl(unsigned_integer_);
        size += sizeof(uint32_t);
        uint32_t aggregated_statistics_deserialize_size = aggregated_statistics_.deserialize(msg_payload, size);
        size += aggregated_statistics_deserialize_size;
        return size - position;
    }
}