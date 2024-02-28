#include "message/key_value_hitflag_utilization_message.h"

namespace covered
{
    const std::string KeyValueHitflagUtilizationMessage::kClassName("KeyValueHitflagUtilizationMessage");

    KeyValueHitflagUtilizationMessage::KeyValueHitflagUtilizationMessage(const Key& key, const Value& value, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        hitflag_ = hitflag;
        cache_size_bytes_ = cache_size_bytes;
        cache_capacity_bytes_ = cache_capacity_bytes;
    }

    KeyValueHitflagUtilizationMessage::KeyValueHitflagUtilizationMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueHitflagUtilizationMessage::~KeyValueHitflagUtilizationMessage() {}

    Key KeyValueHitflagUtilizationMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueHitflagUtilizationMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    Hitflag KeyValueHitflagUtilizationMessage::getHitflag() const
    {
        checkIsValid_();
        return hitflag_;
    }

    uint64_t KeyValueHitflagUtilizationMessage::getCacheSizeBytes() const
    {
        checkIsValid_();
        return cache_size_bytes_;
    }

    uint64_t KeyValueHitflagUtilizationMessage::getCacheCapacityBytes() const
    {
        checkIsValid_();
        return cache_capacity_bytes_;
    }

    uint32_t KeyValueHitflagUtilizationMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + hitflag + cache utilization
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t);
        return msg_payload_size;
    }

    uint32_t KeyValueHitflagUtilizationMessage::getMsgBandwidthSizeInternal_() const
    {
        // key payload + ideal value content size + hitflag + cache utilization
        uint32_t msg_bandwidth_size = key_.getKeyPayloadSize() + value_.getValuesize() + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t);
        return msg_bandwidth_size;
    }

    uint32_t KeyValueHitflagUtilizationMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint8_t hitflag_value = static_cast<uint8_t>(hitflag_);
        msg_payload.deserialize(size, (const char*)&hitflag_value, sizeof(uint8_t));
        size += sizeof(uint8_t);
        msg_payload.deserialize(size, (const char*)&cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.deserialize(size, (const char*)&cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    uint32_t KeyValueHitflagUtilizationMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
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
        msg_payload.serialize(size, (char *)&cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        msg_payload.serialize(size, (char *)&cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }
}