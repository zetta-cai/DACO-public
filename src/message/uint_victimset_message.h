/*
 * UintVictimsetMessage: the base class for messages with an 32-bit unsigned integer and a victim syncset for COVERED's victim synchronization.
 *
 * For CoveredVictimFetchRequest, unsigned_integer_ is an object size for the to-be-admitted object.
 * 
 * By Siyuan Sheng (2023.10.04).
 */

#ifndef UINT_VICTIMSET_MESSAGE_H
#define UINT_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class UintVictimsetMessage : public MessageBase
    {
    public:
        UintVictimsetMessage(const uint32_t& unsigned_integer, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        UintVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~UintVictimsetMessage();

        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        uint32_t unsigned_integer_;
        VictimSyncset victim_syncset_;
    protected:
        uint32_t getUnsignedInteger_() const;
    };
}

#endif