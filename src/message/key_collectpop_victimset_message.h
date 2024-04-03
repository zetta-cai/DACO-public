/*
 * KeyCollectpopVictimsetMessage: the base class for messages each with a key, a collected popularity, and a VictimSyncset.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef KEY_COLLECTPOP_VICTIMSET_MESSAGE_H
#define KEY_COLLECTPOP_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyCollectpopVictimsetMessage : public MessageBase
    {
    public:
        KeyCollectpopVictimsetMessage(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyCollectpopVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyCollectpopVictimsetMessage();

        Key getKey() const;
        const CollectedPopularity& getCollectedPopularityRef() const;
        const VictimSyncset& getVictimSyncsetRef() const;

        // Used by BandwidthUsage
        virtual uint32_t getVictimSyncsetBytes() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        CollectedPopularity collected_popularity_; // For popularity collection
        VictimSyncset victim_syncset_; // For victim synchronization
    };
}

#endif