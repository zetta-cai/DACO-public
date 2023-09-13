#include "message/key_value_hitflag_victimset_message.h"

namespace covered
{
    const std::string KeyValueHitflagVictimsetMessage::kClassName("KeyValueHitflagVictimsetMessage");

    KeyValueHitflagVictimsetMessage::KeyValueHitflagVictimsetMessage(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        hitflag_ = hitflag;
        victim_syncset_ = victim_syncset;
    }

    KeyValueHitflagVictimsetMessage::KeyValueHitflagVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueHitflagVictimsetMessage::~KeyValueHitflagVictimsetMessage() {}

    Key KeyValueHitflagVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueHitflagVictimsetMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    Hitflag KeyValueHitflagVictimsetMessage::getHitflag() const
    {
        checkIsValid_();
        return hitflag_;
    }

    const VictimSyncset& KeyValueHitflagVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyValueHitflagVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + hitflag + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueHitflagVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint8_t hitflag_value = static_cast<uint8_t>(hitflag_);
        msg_payload.deserialize(size, (const char*)&hitflag_value, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyValueHitflagVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint8_t hitflag_value = 0;
        msg_payload.serialize(size, (char *)&hitflag_value, sizeof(uint8_t));
        hitflag_ = static_cast<Hitflag>(hitflag_value);
        size += sizeof(uint8_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}