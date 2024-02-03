/*
 * KeyValueByteSyncinfoMessage: the base class of messages each with a key, a value, a trigger flag, and a BestGuess sync info.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef KEY_VALUE_BYTE_SYNCINFO_MESSAGE_H
#define KEY_VALUE_BYTE_SYNCINFO_MESSAGE_H

#include <string>

#include "cache/bestguess/bestguess_syncinfo.h"
#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueByteSyncinfoMessage : public MessageBase
    {
    public:
        KeyValueByteSyncinfoMessage(const Key& key, const Value& value, const uint8_t& byte, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyValueByteSyncinfoMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueByteSyncinfoMessage();

        Key getKey() const;
        Value getValue() const;
        BestGuessSyncinfo getSyncinfo() const;
    protected:
        uint8_t getByte_() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        uint8_t byte_;
        BestGuessSyncinfo syncinfo_;
    };
}

#endif