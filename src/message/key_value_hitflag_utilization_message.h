/*
 * KeyValueHitflagUtilizationMessage: the base class of messages each with a key, a value, a hit flag, and cache utilization (cache size + cache capacity).
 * 
 * By Siyuan Sheng (2023.07.27).
 */

#ifndef KEY_VALUE_HITFLAG_UTILIZATION_MESSAGE_H
#define KEY_VALUE_HITFLAG_UTILIZATION_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueHitflagUtilizationMessage : public MessageBase
    {
    public:
        KeyValueHitflagUtilizationMessage(const Key& key, const Value& value, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyValueHitflagUtilizationMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueHitflagUtilizationMessage();

        Key getKey() const;
        Value getValue() const;
        Hitflag getHitflag() const;
        uint64_t getCacheSizeBytes() const;
        uint64_t getCacheCapacityBytes() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        Hitflag hitflag_;
        uint64_t cache_size_bytes_;
        uint64_t cache_capacity_bytes_;
    };
}

#endif