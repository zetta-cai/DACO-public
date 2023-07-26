#include "message/control/benchmark/switch_slot_response.h"

namespace covered
{
    const std::string SwitchSlotResponse::kClassName("SwitchSlotResponse");

    SwitchSlotResponse::SwitchSlotResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kSwitchSlotResponse, source_index, source_addr, event_list)
    {
    }

    SwitchSlotResponse::SwitchSlotResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    SwitchSlotResponse::~SwitchSlotResponse() {}
}