#include "message/simple_message.h"

namespace covered
{
    const std::string SimpleMessage::kClassName("SimpleMessage");

    SimpleMessage::SimpleMessage(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    SimpleMessage::SimpleMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    SimpleMessage::~SimpleMessage() {}

    uint32_t SimpleMessage::getMsgPayloadSizeInternal_() const
    {
        return 0;
    }

    uint32_t SimpleMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        return 0;
    }

    uint32_t SimpleMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        return 0;
    }
}