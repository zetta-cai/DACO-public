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
        virtual ~KeyHitflagMessage();

        Key getKey() const;
        Hitflag getHitflag() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Hitflag hitflag_;
    };
}

#endif