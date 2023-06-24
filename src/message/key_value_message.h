/*
 * KeyValueMessage: the base class of messages each with a key and a value.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef KEY_VALUE_MESSAGE_H
#define KEY_VALUE_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueMessage : public MessageBase
    {
    public:
        KeyValueMessage(const Key& key, const Value& value, const MessageType& message_type, const uint32_t& source_index);
        KeyValueMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueMessage();

        Key getKey() const;
        Value getValue() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
    };
}

#endif