/*
 * SimpleFinishrunResponse: a response issued by edge/cloud to acknowledge SimpleFinishrunResponse from evaluator.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef SIMPLE_FINISHRUN_REQUEST_H
#define SIMPLE_FINISHRUN_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class SimpleFinishrunResponse : public SimpleMessage
    {
    public:
        SimpleFinishrunResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        SimpleFinishrunResponse(const DynamicArray& msg_payload);
        virtual ~SimpleFinishrunResponse();
    private:
        static const std::string kClassName;
    };
}

#endif