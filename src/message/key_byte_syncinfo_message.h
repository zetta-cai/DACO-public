/*
 * KeyByteSyncinfoMessage: the base class of messages each with a key, a trigger flag, and a BestGuess sync info.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef KEY_BYTE_SYNCINFO_MESSAGE_H
#define KEY_BYTE_SYNCINFO_MESSAGE_H

#include <string>

#include "cache/bestguess/bestguess_syncinfo.h"
#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyByteSyncinfoMessage : public MessageBase
    {
    public:
        KeyByteSyncinfoMessage(const Key& key, const uint8_t& byte, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyByteSyncinfoMessage(const DynamicArray& msg_payload);
        virtual ~KeyByteSyncinfoMessage();

        Key getKey() const;
        BestGuessSyncinfo getSyncinfo() const;
    protected:
        uint8_t getByte_() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        uint8_t byte_;
        BestGuessSyncinfo syncinfo_;
    };
}

#endif