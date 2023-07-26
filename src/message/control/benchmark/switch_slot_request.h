/*
 * SwitchSlotRequest: a request issued by evaluator to notify switching cur-slot client raw statistics in client.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef SWITCH_SLOT_REQUEST_H
#define SWITCH_SLOT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class SwitchSlotRequest : public SimpleMessage
    {
    public:
        SwitchSlotRequest(const uint32_t& source_index, const NetworkAddr& source_addr);
        SwitchSlotRequest(const DynamicArray& msg_payload);
        virtual ~SwitchSlotRequest();
    private:
        static const std::string kClassName;
    };
}

#endif