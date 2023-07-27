#include "message/control/benchmark/switch_slot_response.h"

namespace covered
{
    const std::string SwitchSlotResponse::kClassName("SwitchSlotResponse");

    SwitchSlotResponse::SwitchSlotResponse(const uint32_t& target_slot_idx, const AggregatedStatisticsBase& aggregated_statistics, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : UintAggstatMessage(target_slot_idx, aggregated_statistics, MessageType::kSwitchSlotResponse, source_index, source_addr, event_list)
    {
    }

    SwitchSlotResponse::SwitchSlotResponse(const DynamicArray& msg_payload) : UintAggstatMessage(msg_payload)
    {
    }

    SwitchSlotResponse::~SwitchSlotResponse() {}

    uint32_t SwitchSlotResponse::getTargetSlotIdx() const
    {
        checkIsValid_();
        return getUnsignedInteger_();
    }
}