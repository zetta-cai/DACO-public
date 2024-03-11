/*
 * FinishWarmupResponse: a response issued by client to acknowledge FinishWarmupRequest from evaluator.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef FINISH_WARMUP_RESPONSE_H
#define FINISH_WARMUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class FinishWarmupResponse : public SimpleMessage
    {
    public:
        FinishWarmupResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum);
        FinishWarmupResponse(const DynamicArray& msg_payload);
        virtual ~FinishWarmupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif