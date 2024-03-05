#include "message/uint_two_aggstat_message.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string UintTwoAggstatMessage::kClassName("UintTwoAggstatMessage");

    UintTwoAggstatMessage::UintTwoAggstatMessage(const uint32_t& unsigned_integer, const AggregatedStatisticsBase& first_aggregated_statistics, const AggregatedStatisticsBase& second_aggregated_statistics, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        unsigned_integer_ = unsigned_integer;
        first_aggregated_statistics_ = first_aggregated_statistics;
        second_aggregated_statistics_ = second_aggregated_statistics;
    }

    UintTwoAggstatMessage::UintTwoAggstatMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    UintTwoAggstatMessage::~UintTwoAggstatMessage() {}

    uint32_t UintTwoAggstatMessage::getUnsignedInteger_() const
    {
        checkIsValid_();
        return unsigned_integer_;
    }

    AggregatedStatisticsBase UintTwoAggstatMessage::getFirstAggregatedStatistics_() const
    {
        checkIsValid_();
        return first_aggregated_statistics_;
    }

    AggregatedStatisticsBase UintTwoAggstatMessage::getSecondAggregatedStatistics_() const
    {
        checkIsValid_();
        return second_aggregated_statistics_;
    }

    uint32_t UintTwoAggstatMessage::getMsgPayloadSizeInternal_() const
    {
        // unsigned integer + two aggregated statistics
        return sizeof(uint32_t) + first_aggregated_statistics_.getAggregatedStatisticsIOSize() + second_aggregated_statistics_.getAggregatedStatisticsIOSize();
    }

    uint32_t UintTwoAggstatMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_unsigned_integer = htonl(unsigned_integer_);
        msg_payload.deserialize(size, (const char*)&bigendian_unsigned_integer, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t first_aggregated_statistics_serialize_size = first_aggregated_statistics_.serialize(msg_payload, size);
        size += first_aggregated_statistics_serialize_size;
        uint32_t second_aggregated_statistics_serialize_size = second_aggregated_statistics_.serialize(msg_payload, size);
        size += second_aggregated_statistics_serialize_size;
        return size - position;
    }

    uint32_t UintTwoAggstatMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&unsigned_integer_, sizeof(uint32_t));
        unsigned_integer_ = ntohl(unsigned_integer_);
        size += sizeof(uint32_t);
        uint32_t first_aggregated_statistics_deserialize_size = first_aggregated_statistics_.deserialize(msg_payload, size);
        size += first_aggregated_statistics_deserialize_size;
        uint32_t second_aggregated_statistics_deserialize_size = second_aggregated_statistics_.deserialize(msg_payload, size);
        size += second_aggregated_statistics_deserialize_size;
        return size - position;
    }
}