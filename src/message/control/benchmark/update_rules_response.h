/*
 * UpdateRulesResponse: a response issued by client to acknowledge UpdateRulesRequest from evaluator.
 * 
 * By Siyuan Sheng (2024.11.07).
 */

#ifndef UPDATE_RULES_RESPONSE_H
#define UPDATE_RULES_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class UpdateRulesResponse : public SimpleMessage
    {
    public:
        UpdateRulesResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum);
        UpdateRulesResponse(const DynamicArray& msg_payload);
        virtual ~UpdateRulesResponse();
    private:
        static const std::string kClassName;
    };
}

#endif