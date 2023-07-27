/*
 * KeyByteMessage: the base class for messages each with a key and a single byte.
 *
 * ~~(1) For LocalDelResponse and LocalPutResponse, the byte is a cache hit flag.~~ (use KeyHitflagUtilizationMessage now).
 * (2) For DirectoryUpdateResponse, the byte is a write flag.
 * (3) For AcquireWritelockResponse, the byte is a success flag.
 * 
 * By Siyuan Sheng (2023.05.23).
 */

#ifndef KEY_BYTE_MESSAGE_H
#define KEY_BYTE_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyByteMessage : public MessageBase
    {
    public:
        KeyByteMessage(const Key& key, const uint8_t& byte, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        KeyByteMessage(const DynamicArray& msg_payload);
        virtual ~KeyByteMessage();

        Key getKey() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        uint8_t byte_;
    protected:
        uint8_t getByte_() const;
    };
}

#endif