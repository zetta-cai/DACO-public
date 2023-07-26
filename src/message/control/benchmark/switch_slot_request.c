#include "message/control/benchmark/switch_slot_request.h"

namespace covered
{
    const std::string SwitchSlotRequest::kClassName("SwitchSlotRequest");

    SwitchSlotRequest::SwitchSlotRequest(const uint32_t& source_index, const NetworkAddr& source_addr) : SimpleMessage(MessageType::kSwitchSlotRequest, source_index, source_addr, EventList())
    {
    }

    SwitchSlotRequest::SwitchSlotRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    SwitchSlotRequest::~SwitchSlotRequest() {}
}