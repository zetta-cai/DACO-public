/*
 * KeyVictimsetMessage: the base class for messages each with a key and a VictimSyncset.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef KEY_VICTIMSET_MESSAGE_H
#define KEY_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyVictimsetMessage : public MessageBase
    {
    public:
        KeyVictimsetMessage(const Key& key, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyVictimsetMessage();

        Key getKey() const;
        const VictimSyncset& getVictimSyncsetRef() const;

        // Used by BandwidthUsage
        virtual uint32_t getVictimSyncsetBytes() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        VictimSyncset victim_syncset_; // For victim synchronization
    };
}

#endif