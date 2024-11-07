/*
 * UpdateRulesRequest: a request issued by evaluator to notify the client to update dynamic rules.
 * 
 * By Siyuan Sheng (2024.11.07).
 */

#ifndef UPDATE_RULES_REQUEST_H
#define UPDATE_RULES_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class UpdateRulesRequest : public SimpleMessage
    {
    public:
        UpdateRulesRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum);
        UpdateRulesRequest(const DynamicArray& msg_payload);
        virtual ~UpdateRulesRequest();
    private:
        static const std::string kClassName;
    };
}

#endif