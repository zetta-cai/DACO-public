/*
 * FinishWarmupRequest: a request issued by evaluator to notify the end of warmup phase to client.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef FINISH_WARMUP_REQUEST_H
#define FINISH_WARMUP_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class FinishWarmupRequest : public SimpleMessage
    {
    public:
        FinishWarmupRequest(const uint32_t& source_index, const NetworkAddr& source_addr);
        FinishWarmupRequest(const DynamicArray& msg_payload);
        virtual ~FinishWarmupRequest();
    private:
        static const std::string kClassName;
    };
}

#endif