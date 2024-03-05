#include "message/uint_message.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string UintMessage::kClassName("UintMessage");

    UintMessage::UintMessage(const uint32_t& unsigned_integer, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        unsigned_integer_ = unsigned_integer;
    }

    UintMessage::UintMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    UintMessage::~UintMessage() {}

    uint32_t UintMessage::getUnsignedInteger_() const
    {
        checkIsValid_();
        return unsigned_integer_;
    }

    uint32_t UintMessage::getMsgPayloadSizeInternal_() const
    {
        // unsigned integer
        return sizeof(uint32_t);
    }

    uint32_t UintMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_unsigned_integer = htonl(unsigned_integer_);
        msg_payload.deserialize(size, (const char*)&bigendian_unsigned_integer, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t UintMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&unsigned_integer_, sizeof(uint32_t));
        unsigned_integer_ = ntohl(unsigned_integer_);
        size += sizeof(uint32_t);
        return size - position;
    }
}