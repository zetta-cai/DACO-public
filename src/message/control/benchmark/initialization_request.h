/*
 * InitializationRequest: a request issued by client/edge/cloud to notify the end of initialization to evaluator.
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef INITIALIZATION_REQUEST_H
#define INITIALIZATION_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class InitializationRequest : public SimpleMessage
    {
    public:
        InitializationRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum);
        InitializationRequest(const DynamicArray& msg_payload);
        virtual ~InitializationRequest();
    private:
        static const std::string kClassName;
    };
}

#endif