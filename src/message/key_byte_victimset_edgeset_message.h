/*
 * KeyByteVictimsetEdgesetMessage: the base class for messages each with a key, a single byte, a victim syncset, and an edgeset for COVERED.
 *
 * For CoveredPlacementDirectoryEvictResponse, the byte is a write flag.
 * 
 * By Siyuan Sheng (2023.10.11).
 */

#ifndef KEY_BYTE_VICIMSET_EDGESET_MESSAGE_H
#define KEY_BYTE_VICIMSET_EDGESET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/victim/victim_syncset.h"
#include "core/popularity/edgeset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyByteVictimsetEdgesetMessage : public MessageBase
    {
    public:
        KeyByteVictimsetEdgesetMessage(const Key& key, const uint8_t& byte, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        KeyByteVictimsetEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyByteVictimsetEdgesetMessage();

        Key getKey() const;
        const VictimSyncset& getVictimSyncsetRef() const;
        const Edgeset& getEdgesetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        uint8_t byte_;
        VictimSyncset victim_syncset_; // For victim synchronization
        Edgeset edgeset_; // For placement deployment
    protected:
        uint8_t getByte_() const;
    };
}

#endif