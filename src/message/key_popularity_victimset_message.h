/*
 * KeyPopularityVictimsetMessage: the base class for messages each with a key, a local uncached popularity, and a VictimSyncset.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef KEY_POPULARITY_VICTIMSET_MESSAGE_H
#define KEY_POPULARITY_VICTIMSET_MESSAGE_H

#include <string>

#include "cache/covered/common_header.h"
#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyPopularityVictimsetMessage : public MessageBase
    {
    public:
        KeyPopularityVictimsetMessage(const Key& key, const Popularity& local_uncached_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        KeyPopularityVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyPopularityVictimsetMessage();

        Key getKey() const;
        Popularity getLocalUncachedPopularity() const;
        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Popularity local_uncached_popularity_; // For popularity collection
        VictimSyncset victim_syncset_; // For victim synchronization
    };
}

#endif