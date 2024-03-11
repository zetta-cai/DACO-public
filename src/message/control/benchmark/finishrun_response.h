/*
 * FinishrunResponse: a response issued by client to acknowledge FinishrunRequest from evaluator.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef FINISHRUN_RESPONSE_H
#define FINISHRUN_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/uint_two_aggstat_message.h"

namespace covered
{
    class FinishrunResponse : public UintTwoAggstatMessage
    {
    public:
        FinishrunResponse(const uint32_t& last_slot_idx, const AggregatedStatisticsBase& last_slot_aggregated_statistics, const AggregatedStatisticsBase& stable_aggregated_statistics, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum);
        FinishrunResponse(const DynamicArray& msg_payload);
        virtual ~FinishrunResponse();

        uint32_t getLastSlotIdx() const;
        AggregatedStatisticsBase getLastSlotAggregatedStatistics() const;
        AggregatedStatisticsBase getStableAggregatedStatistics() const;
    private:
        static const std::string kClassName;
    };
}

#endif