/*
 * KeyVictimsetEdgesetMessage: the base class for messages each with a key, a VictimSyncset, and a set of edge indexes.
 * 
 * By Siyuan Sheng (2023.09.24).
 */

#ifndef KEY_VICTIMSET_EDGESET_MESSAGE_H
#define KEY_VICTIMSET_EDGESET_MESSAGE_H

#include <string>
#include <unordered_set>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/popularity/edgeset.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyVictimsetEdgesetMessage : public MessageBase
    {
    public:
        KeyVictimsetEdgesetMessage(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyVictimsetEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyVictimsetEdgesetMessage();

        Key getKey() const;
        const VictimSyncset& getVictimSyncsetRef() const;
        const Edgeset& getEdgesetRef() const;

        // Used by BandwidthUsage
        virtual uint32_t getVictimSyncsetBytes() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        VictimSyncset victim_syncset_; // For victim synchronization
        Edgeset edgeset_; // For non-blocking placement deployment
    };
}

#endif