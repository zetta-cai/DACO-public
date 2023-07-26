/*
 * StartrunResponse: a request issued by client/edge/cloud to notify the end of initialization to evaluator.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef STARTRUN_RESPONSE_H
#define STARTRUN_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class StartrunResponse : public SimpleMessage
    {
    public:
        StartrunResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        StartrunResponse(const DynamicArray& msg_payload);
        virtual ~StartrunResponse();
    private:
        static const std::string kClassName;
    };
}

#endif