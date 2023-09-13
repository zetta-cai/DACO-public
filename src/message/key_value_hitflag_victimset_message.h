/*
 * KeyValueHitflagVictimsetMessage: the base class of messages each with a key, a value, a hit flag, and a victim syncset.
 * 
 * By Siyuan Sheng (2023.09.13).
 */

#ifndef KEY_VALUE_HITFLAG_VICTIMSET_MESSAGE_H
#define KEY_VALUE_HITFLAG_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueHitflagVictimsetMessage : public MessageBase
    {
    public:
        KeyValueHitflagVictimsetMessage(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        KeyValueHitflagVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueHitflagVictimsetMessage();

        Key getKey() const;
        Value getValue() const;
        Hitflag getHitflag() const;
        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        Hitflag hitflag_;
        VictimSyncset victim_syncset_; // For victim synchronization
    };
}

#endif