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
        KeyMessage(const Key& key, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyMessage(const DynamicArray& msg_payload);
        virtual ~KeyMessage();

        Key getKey() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
    };
}

#endif