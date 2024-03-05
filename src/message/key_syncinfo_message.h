/*
 * KeySyncinfoMessage: the base class of messages each with a key and a BestGuess sync info.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef KEY_SYNCINFO_MESSAGE_H
#define KEY_SYNCINFO_MESSAGE_H

#include <string>

#include "cache/bestguess/bestguess_syncinfo.h"
#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeySyncinfoMessage : public MessageBase
    {
    public:
        KeySyncinfoMessage(const Key& key, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeySyncinfoMessage(const DynamicArray& msg_payload);
        virtual ~KeySyncinfoMessage();

        Key getKey() const;
        BestGuessSyncinfo getSyncinfo() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        BestGuessSyncinfo syncinfo_;
    };
}

#endif