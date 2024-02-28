/*
 * KeyValueByteVictimsetMessage: the base class of messages each with a key, a value, a single byte, and a victim syncset.
 *
 * (1) For CoveredRedirectedGetResponse, the single byte is a hitflag.
 * (2) For CoveredBgplacePlacementNotifyRequest, the single byte is validity for value.
 * 
 * By Siyuan Sheng (2023.09.13).
 */

#ifndef KEY_VALUE_BYTE_VICTIMSET_MESSAGE_H
#define KEY_VALUE_BYTE_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValueByteVictimsetMessage : public MessageBase
    {
    public:
        KeyValueByteVictimsetMessage(const Key& key, const Value& value, const uint8_t& byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyValueByteVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyValueByteVictimsetMessage();

        Key getKey() const;
        Value getValue() const;
        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;
        virtual uint32_t getMsgBandwidthSizeInternal_() const override; // Override due to with value content

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
        uint8_t byte_;
        VictimSyncset victim_syncset_; // For victim synchronization
    protected:
        uint8_t getByte_() const;
    };
}

#endif