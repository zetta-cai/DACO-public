/*
 * UintTwoAggstatMessage: the base class for messages with an 32-bit unsigned integer and two aggregated statistics.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef UINT_TWO_AGGSTAT_MESSAGE_H
#define UINT_TWO_AGGSTAT_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/message_base.h"
#include "statistics/aggregated_statistics_base.h"

namespace covered
{
    class UintTwoAggstatMessage : public MessageBase
    {
    public:
        UintTwoAggstatMessage(const uint32_t& unsigned_integer, const AggregatedStatisticsBase& first_aggregated_statistics, const AggregatedStatisticsBase& second_aggregated_statistics, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        UintTwoAggstatMessage(const DynamicArray& msg_payload);
        virtual ~UintTwoAggstatMessage();
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        uint32_t unsigned_integer_;
        AggregatedStatisticsBase first_aggregated_statistics_;
        AggregatedStatisticsBase second_aggregated_statistics_;
    protected:
        uint32_t getUnsignedInteger_() const;
        AggregatedStatisticsBase getFirstAggregatedStatistics_() const;
        AggregatedStatisticsBase getSecondAggregatedStatistics_() const;
    };
}

#endif