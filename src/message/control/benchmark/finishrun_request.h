/*
 * FinishrunRequest: a request issued by evaluator to notify the end of benchmark (i.e., stresstest phase) to client.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef FINISHRUN_REQUEST_H
#define FINISHRUN_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class FinishrunRequest : public SimpleMessage
    {
    public:
        FinishrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr);
        FinishrunRequest(const DynamicArray& msg_payload);
        virtual ~FinishrunRequest();
    private:
        static const std::string kClassName;
    };
}

#endif