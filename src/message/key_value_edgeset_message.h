/*
 * KeyValueEdgesetMessage: the base class of messages each with a key and a value.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef KEY_VALUE_EDGESET_MESSAGE_H
#define KEY_VALUE_EDGESET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "core/popularity/edgeset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueEdgesetMessage : public MessageBase
    {
    public:
        KeyValueEdgesetMessage(const Key& key, const Value& value, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValueEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueEdgesetMessage();

        Key getKey() const;
        Value getValue() const;
        const Edgeset& getEdgesetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;
        virtual uint32_t getMsgBandwidthSizeInternal_() const override; // Override due to with value content

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        Edgeset edgeset_;
    };
}

#endif