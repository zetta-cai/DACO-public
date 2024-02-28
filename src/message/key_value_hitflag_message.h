/*
 * KeyValueHitflagMessage: the base class of messages each with a key, a value, and a hit flag.
 * 
 * By Siyuan Sheng (2023.05.23).
 */

#ifndef KEY_VALUE_HITFLAG_MESSAGE_H
#define KEY_VALUE_HITFLAG_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueHitflagMessage : public MessageBase
    {
    public:
        KeyValueHitflagMessage(const Key& key, const Value& value, const Hitflag& hitflag, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyValueHitflagMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueHitflagMessage();

        Key getKey() const;
        Value getValue() const;
        Hitflag getHitflag() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;
        virtual uint32_t getMsgBandwidthSizeInternal_() const override; // Override due to with value content

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        Hitflag hitflag_;
    };
}

#endif