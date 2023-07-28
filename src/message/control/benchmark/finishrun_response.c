#include "message/control/benchmark/finishrun_response.h"

namespace covered
{
    const std::string FinishrunResponse::kClassName("FinishrunResponse");

    FinishrunResponse::FinishrunResponse(const uint32_t& last_slot_idx, const AggregatedStatisticsBase& last_slot_aggregated_statistics, const AggregatedStatisticsBase& stable_aggregated_statistics, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : UintTwoAggstatMessage(last_slot_idx, last_slot_aggregated_statistics, stable_aggregated_statistics, MessageType::kFinishrunResponse, source_index, source_addr, event_list)
    {
    }

    FinishrunResponse::FinishrunResponse(const DynamicArray& msg_payload) : UintTwoAggstatMessage(msg_payload)
    {
    }

    FinishrunResponse::~FinishrunResponse() {}

    uint32_t FinishrunResponse::getLastSlotIdx() const
    {
        return getUnsignedInteger_();
    }

    AggregatedStatisticsBase FinishrunResponse::getLastSlotAggregatedStatistics() const
    {
        return getFirstAggregatedStatistics_();
    }

    AggregatedStatisticsBase FinishrunResponse::getStableAggregatedStatistics() const
    {
        return getSecondAggregatedStatistics_();
    }
}