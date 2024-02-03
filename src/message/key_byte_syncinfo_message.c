#include "message/key_byte_syncinfo_message.h"

namespace covered
{
    const std::string KeyByteSyncinfoMessage::kClassName("KeyByteSyncinfoMessage");

    KeyByteSyncinfoMessage::KeyByteSyncinfoMessage(const Key& key, const uint8_t& byte, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        byte_ = byte;
        syncinfo_ = syncinfo;
    }

    KeyByteSyncinfoMessage::KeyByteSyncinfoMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyByteSyncinfoMessage::~KeyByteSyncinfoMessage() {}

    Key KeyByteSyncinfoMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint8_t KeyByteSyncinfoMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    BestGuessSyncinfo KeyByteSyncinfoMessage::getSyncinfo() const
    {
        checkIsValid_();
        return syncinfo_;
    }

    uint32_t KeyByteSyncinfoMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + byte + syncinfo
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + syncinfo_.getSyncinfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyByteSyncinfoMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t syncinfo_serialize_size = syncinfo_.serialize(msg_payload, size);
        size += syncinfo_serialize_size;
        return size - position;
    }

    uint32_t KeyByteSyncinfoMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t syncinfo_deserialize_size = syncinfo_.deserialize(msg_payload, size);
        size += syncinfo_deserialize_size;
        return size - position;
    }
}