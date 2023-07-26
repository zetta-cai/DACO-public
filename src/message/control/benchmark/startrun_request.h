/*
 * StartrunRequest: a request issued by evaluator to notify the start of benchmark (i.e., warmup phase) to client/edge/cloud.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef STARTRUN_REQUEST_H
#define STARTRUN_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class StartrunRequest : public SimpleMessage
    {
    public:
        StartrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr);
        StartrunRequest(const DynamicArray& msg_payload);
        virtual ~StartrunRequest();
    private:
        static const std::string kClassName;
    };
}

#endif