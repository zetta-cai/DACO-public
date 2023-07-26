/*
 * SimpleMessage: the base class for simple messages without any extra field.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef SIMPLE_MESSAGE_H
#define SIMPLE_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/message_base.h"

namespace covered
{
    class SimpleMessage : public MessageBase
    {
    public:
        SimpleMessage(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        SimpleMessage(const DynamicArray& msg_payload);
        virtual ~SimpleMessage();
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;
    };
}

#endif