/*
 * KeyByteVictimsetMessage: the base class for messages each with a key, a single byte, and a victim syncset for COVERED.
 *
 * For CoveredAcquireWritelockResponse, the byte is a success flag.
 * 
 * By Siyuan Sheng (2023.09.08).
 */

#ifndef KEY_BYTE_VICIMSET_MESSAGE_H
#define KEY_BYTE_VICIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyByteVictimsetMessage : public MessageBase
    {
    public:
        KeyByteVictimsetMessage(const Key& key, const uint8_t& byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyByteVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyByteVictimsetMessage();

        Key getKey() const;
        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        uint8_t byte_;
        VictimSyncset victim_syncset_; // For victim synchronization
    protected:
        uint8_t getByte_() const;
    };
}

#endif