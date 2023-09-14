/*
 * UintAggstatMessage: the base class for messages with an 32-bit unsigned integer and aggregated statistics.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef UINT_AGGSTAT_MESSAGE_H
#define UINT_AGGSTAT_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/message_base.h"
#include "statistics/aggregated_statistics_base.h"

namespace covered
{
    class UintAggstatMessage : public MessageBase
    {
    public:
        UintAggstatMessage(const uint32_t& unsigned_integer, const AggregatedStatisticsBase& aggregated_statistics, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        UintAggstatMessage(const DynamicArray& msg_payload);
        virtual ~UintAggstatMessage();

        AggregatedStatisticsBase getAggregatedStatistics() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        uint32_t unsigned_integer_;
        AggregatedStatisticsBase aggregated_statistics_;
    protected:
        uint32_t getUnsignedInteger_() const;
    };
}

#endif