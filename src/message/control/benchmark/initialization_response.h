/*
 * InitializationResponse: a response issued by evaluator to acknowledge the initialization requests from client/edge/cloud.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef INITIALIZATION_RESPONSE_H
#define INITIALIZATION_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class InitializationResponse : public SimpleMessage
    {
    public:
        InitializationResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        InitializationResponse(const DynamicArray& msg_payload);
        virtual ~InitializationResponse();
    private:
        static const std::string kClassName;
    };
}

#endif