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
        KeyValueHitflagMessage(const Key& key, const Value& value, const Hitflag& hitflag, const MessageType& message_type);
        KeyValueHitflagMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueHitflagMessage();

        Key getKey() const;
        Value getValue() const;
        Hitflag getHitflag() const;

        virtual uint32_t getMsgPayloadSize() override;
    private:
        static const std::string kClassName;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        const Key key_;
        const Value value_;
        const Hitflag hitflag_;
    };
}

#endif