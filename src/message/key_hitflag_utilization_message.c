#include "message/key_hitflag_utilization_message.h"

namespace covered
{
    const std::string KeyHitflagUtilizationMessage::kClassName("KeyHitflagUtilizationMessage");

    KeyHitflagUtilizationMessage::KeyHitflagUtilizationMessage(const Key& key, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        key_ = key;
        hitflag_ = hitflag;
        cache_size_bytes_ = cache_size_bytes;
        cache_capacity_bytes_ = cache_capacity_bytes;
    }

    KeyHitflagUtilizationMessage::KeyHitflagUtilizationMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyHitflagUtilizationMessage::~KeyHitflagUtilizationMessage() {}

    Key KeyHitflagUtilizationMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Hitflag KeyHitflagUtilizationMessage::getHitflag() const
    {
        checkIsValid_();
        return hitflag_;
    }

    uint64_t KeyHitflagUtilizationMessage::getCacheSizeBytes() const
    {
        checkIsValid_();
        return cache_size_bytes_;
    }

    uint64_t KeyHitflagUtilizationMessage::getCacheCapacityBytes() const
    {
        checkIsValid_();
        return cache_capacity_bytes_;
    }

    uint32_t KeyHitflagUtilizationMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + hit flag + cache size + cache capacity
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t);
        return msg_payload_size;
    }

    uint32_t KeyHitflagUtilizationMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint8_t hitflag_value = static_cast<uint8_t>(hitflag_);
        msg_payload.deserialize(size, (const char*)&hitflag_value, sizeof(uint8_t));
        size += sizeof(uint8_t);
        msg_payload.deserialize(size, (const char*)&cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    uint32_t KeyHitflagUtilizationMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint8_t hitflag_value = 0;
        msg_payload.serialize(size, (char *)&hitflag_value, sizeof(uint8_t));
        hitflag_ = static_cast<Hitflag>(hitflag_value);
        size += sizeof(uint8_t);
        msg_payload.serialize(size, (char *)&cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }
}