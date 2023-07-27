#include "message/control/benchmark/switch_slot_request.h"

namespace covered
{
    const std::string SwitchSlotRequest::kClassName("SwitchSlotRequest");

    SwitchSlotRequest::SwitchSlotRequest(const uint32_t& target_slot_idx, const uint32_t& source_index, const NetworkAddr& source_addr) : UintMessage(target_slot_idx, MessageType::kSwitchSlotRequest, source_index, source_addr, EventList())
    {
    }

    SwitchSlotRequest::SwitchSlotRequest(const DynamicArray& msg_payload) : UintMessage(msg_payload)
    {
    }

    SwitchSlotRequest::~SwitchSlotRequest() {}

    uint32_t SwitchSlotRequest::getTargetSlotIdx() const
    {
        checkIsValid_();
        return getUnsignedInteger_();
    }
}