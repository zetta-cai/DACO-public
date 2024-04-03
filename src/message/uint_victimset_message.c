#include "message/uint_victimset_message.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string UintVictimsetMessage::kClassName("UintVictimsetMessage");

    UintVictimsetMessage::UintVictimsetMessage(const uint32_t& unsigned_integer, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        unsigned_integer_ = unsigned_integer;
        victim_syncset_ = victim_syncset;
    }

    UintVictimsetMessage::UintVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    UintVictimsetMessage::~UintVictimsetMessage() {}

    const VictimSyncset& UintVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t UintVictimsetMessage::getVictimSyncsetBytes() const
    {
        checkIsValid_();
        return victim_syncset_.getVictimSyncsetPayloadSize();
    }

    uint32_t UintVictimsetMessage::getUnsignedInteger_() const
    {
        checkIsValid_();
        return unsigned_integer_;
    }

    uint32_t UintVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // unsigned integer + victim syncset
        return sizeof(uint32_t) + victim_syncset_.getVictimSyncsetPayloadSize();
    }

    uint32_t UintVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_unsigned_integer = htonl(unsigned_integer_);
        msg_payload.deserialize(size, (const char*)&bigendian_unsigned_integer, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t UintVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&unsigned_integer_, sizeof(uint32_t));
        unsigned_integer_ = ntohl(unsigned_integer_);
        size += sizeof(uint32_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}