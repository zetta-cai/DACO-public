/*
 * UintMessage: the base class for messages with an 32-bit unsigned integer.
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
        UintMessage(const uint32_t& unsigned_integer, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
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