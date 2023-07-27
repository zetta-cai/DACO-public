/*
 * SwitchSlotResponse: a response issued by client to acknowledge SwitchSlotRequest from evaluator.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef SWITCH_SLOT_RESPONSE_H
#define SWITCH_SLOT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/uint_aggstat_message.h"

namespace covered
{
    class SwitchSlotResponse : public UintAggstatMessage
    {
    public:
        SwitchSlotResponse(const uint32_t& target_slot_idx, const AggregatedStatisticsBase& aggregated_statistics, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        SwitchSlotResponse(const DynamicArray& msg_payload);
        virtual ~SwitchSlotResponse();

        uint32_t getTargetSlotIdx() const;
    private:
        static const std::string kClassName;
    };
}

#endif