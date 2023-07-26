/*
 * SwitchSlotResponse: a response issued by client to acknowledge SwitchSlotRequest from evaluator.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef SWITCH_SLOT_RESPONSE_H
#define SWITCH_SLOT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class SwitchSlotResponse : public SimpleMessage
    {
    public:
        SwitchSlotResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        SwitchSlotResponse(const DynamicArray& msg_payload);
        virtual ~SwitchSlotResponse();
    private:
        static const std::string kClassName;
    };
}

#endif