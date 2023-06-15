/*
 * KeyWriteflagMessage: the base class for messages each with a a write flag.
 *
 * NOTE: the write flag indicates whether key is being written.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef KEY_WRITEFLAG_MESSAGE_H
#define KEY_WRITEFLAG_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyWriteflagMessage : public MessageBase
    {
    public:
        KeyWriteflagMessage(const Key& key, const bool& is_being_written, const MessageType& message_type);
        KeyWriteflagMessage(const DynamicArray& msg_payload);
        virtual ~KeyWriteflagMessage();

        Key getKey() const;
        bool isBeingWritten() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        bool is_being_written_;
    };
}

#endif