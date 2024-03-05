#include "message/simple_message.h"

namespace covered
{
    const std::string SimpleMessage::kClassName("SimpleMessage");

    SimpleMessage::SimpleMessage(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
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