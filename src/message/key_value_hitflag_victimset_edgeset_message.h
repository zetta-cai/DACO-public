/*
 * KeyValueHitflagVictimsetEdgesetMessage: the base class of messages each with a key, a value, a hit flag, a victim syncset, and an edgeset.
 * 
 * By Siyuan Sheng (2023.09.25).
 */

#ifndef KEY_VALUE_HITFLAG_VICTIMSET_EDGESET_MESSAGE_H
#define KEY_VALUE_HITFLAG_VICTIMSET_EDGESET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "core/popularity/edgeset.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueHitflagVictimsetEdgesetMessage : public MessageBase
    {
    public:
        KeyValueHitflagVictimsetEdgesetMessage(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValueHitflagVictimsetEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueHitflagVictimsetEdgesetMessage();

        Key getKey() const;
        Value getValue() const;
        Hitflag getHitflag() const;
        const VictimSyncset& getVictimSyncsetRef() const;
        const Edgeset& getEdgesetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;
        virtual uint32_t getMsgBandwidthSizeInternal_() const override; // Override due to with value content

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        Hitflag hitflag_;
        VictimSyncset victim_syncset_; // For victim synchronization
        Edgeset edgeset_; // For non-blocking placement deployment
    };
}

#endif