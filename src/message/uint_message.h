/*
 * UintMessage: the base class for messages with an 32-bit unsigned integer.
 *
 * (1) For SwitchSlotRequest, unsigned_integer_ is a target measurement slot index.
 * (2) For CoveredVictimFetchRequest, unsigned_integer_ is an object size for the to-be-admitted object.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef UINT_MESSAGE_H
#define UINT_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/message_base.h"

namespace covered
{
    class UintMessage : public MessageBase
    {
    public:
        UintMessage(const uint32_t& unsigned_integer, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        UintMessage(const DynamicArray& msg_payload);
        virtual ~UintMessage();
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        uint32_t unsigned_integer_;
    protected:
        uint32_t getUnsignedInteger_() const;
    };
}

#endif