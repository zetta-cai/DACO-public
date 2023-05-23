/*
 * KeyHitflagMessage: the base class for messages each with a key and a hit flag.
 * 
 * By Siyuan Sheng (2023.05.23).
 */

#ifndef KEY_HITFLAG_MESSAGE_H
#define KEY_HITFLAG_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyHitflagMessage : public MessageBase
    {
    public:
        KeyHitflagMessage(const Key& key, const Hitflag& hitflag, const MessageType& message_type);
        KeyHitflagMessage(const DynamicArray& msg_payload);
        ~KeyHitflagMessage();

        Key getKey() const;
        Hitflag getHitflag() const;

        virtual uint32_t getMsgPayloadSize() override;
    private:
        static const std::string kClassName;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        const Key key_;
        const Hitflag hitflag_;
    };
}

#endif