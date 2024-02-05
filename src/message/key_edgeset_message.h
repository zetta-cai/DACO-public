/*
 * KeyEdgesetMessage: the base class for messages each with a key and an edgeset.
 * 
 * By Siyuan Sheng (2023.09.27).
 */

#ifndef KEY_EDGESET_MESSAGE_H
#define KEY_EDGESET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/popularity/edgeset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyEdgesetMessage : public MessageBase
    {
    public:
        KeyEdgesetMessage(const Key& key, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyEdgesetMessage();

        Key getKey() const;
        const Edgeset& getEdgesetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Edgeset edgeset_;
    };
}

#endif