/*
 * KeyValuePlaceinfoSyncinfoMessage: the base class of messages each with a key, a value, a BestGuess placement info, and a BestGuess sync info.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef KEY_VALUE_PLACEINFO_SYNCINFO_MESSAGE_H
#define KEY_VALUE_PLACEINFO_SYNCINFO_MESSAGE_H

#include <string>

#include "cache/bestguess/bestguess_placeinfo.h"
#include "cache/bestguess/bestguess_syncinfo.h"
#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValuePlaceinfoSyncinfoMessage : public MessageBase
    {
    public:
        KeyValuePlaceinfoSyncinfoMessage(const Key& key, const Value& value, const BestGuessPlaceinfo& placeinfo, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValuePlaceinfoSyncinfoMessage(const DynamicArray& msg_payload);
        virtual ~KeyValuePlaceinfoSyncinfoMessage();

        Key getKey() const;
        Value getValue() const;
        BestGuessPlaceinfo getPlaceinfo() const;
        BestGuessSyncinfo getSyncinfo() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;
        virtual uint32_t getMsgBandwidthSizeInternal_() const override; // Override due to with value content

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        BestGuessPlaceinfo placeinfo_;
        BestGuessSyncinfo syncinfo_;
    };
}

#endif