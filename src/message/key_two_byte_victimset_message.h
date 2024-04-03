/*
 * KeyTwoByteVictimsetMessage: the base class for messages each with a key, a single byte, and a victim syncset for COVERED.
 *
 * For CoveredDirectoryUpdateResponse and CoveredBgplaceDirectoryUpdateResponse, the first byte is a write flag and the second byte is a neighbor cached flag.
 * 
 * By Siyuan Sheng (2023.11.26).
 */

#ifndef KEY_TWO_BYTE_VICIMSET_MESSAGE_H
#define KEY_TWO_BYTE_VICIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyTwoByteVictimsetMessage : public MessageBase
    {
    public:
        KeyTwoByteVictimsetMessage(const Key& key, const uint8_t& first_byte, const uint8_t& second_byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyTwoByteVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyTwoByteVictimsetMessage();

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
        uint8_t first_byte_;
        uint8_t second_byte_;
        VictimSyncset victim_syncset_; // For victim synchronization
    protected:
        uint8_t getFirstByte_() const;
        uint8_t getSecondByte_() const;
    };
}

#endif