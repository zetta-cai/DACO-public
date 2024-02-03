#include "message/key_value_byte_syncinfo_message.h"

namespace covered
{
    const std::string KeyValueByteSyncinfoMessage::kClassName("KeyValueByteSyncinfoMessage");

    KeyValueByteSyncinfoMessage::KeyValueByteSyncinfoMessage(const Key& key, const Value& value, const uint8_t& byte, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        byte_ = byte;
        syncinfo_ = syncinfo;
    }

    KeyValueByteSyncinfoMessage::KeyValueByteSyncinfoMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueByteSyncinfoMessage::~KeyValueByteSyncinfoMessage() {}

    Key KeyValueByteSyncinfoMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueByteSyncinfoMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    uint8_t KeyValueByteSyncinfoMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    BestGuessSyncinfo KeyValueByteSyncinfoMessage::getSyncinfo() const
    {
        checkIsValid_();
        return syncinfo_;
    }

    uint32_t KeyValueByteSyncinfoMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + byte + syncinfo
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + sizeof(bool) + syncinfo_.getSyncinfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueByteSyncinfoMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t syncinfo_serialize_size = syncinfo_.serialize(msg_payload, size);
        size += syncinfo_serialize_size;
        return size - position;
    }

    uint32_t KeyValueByteSyncinfoMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t syncinfo_deserialize_size = syncinfo_.deserialize(msg_payload, size);
        size += syncinfo_deserialize_size;
        return size - position;
    }
}