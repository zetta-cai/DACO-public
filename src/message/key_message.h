/*
 * KeyMessage: the base class for messages each with a key.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef KEY_MESSAGE_H
#define KEY_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyMessage : public MessageBase
    {
    public:
        KeyMessage(const Key& key, const MessageType& message_type);
        KeyMessage(const DynamicArray& msg_payload);
        ~KeyMessage();

        Key getKey() const;

        virtual uint32_t getMsgPayloadSize() override;
    private:
        static const std::string kClassName;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        const Key key_;
    };
}

#endif