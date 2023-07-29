/*
 * GlobalDelResponse: a response issued by cloud to an edge node for GlobalDelRequest.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_DEL_RESPONSE_H
#define GLOBAL_DEL_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class GlobalDelResponse : public KeyMessage
    {
    public:
        GlobalDelResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        GlobalDelResponse(const DynamicArray& msg_payload);
        virtual ~GlobalDelResponse();
    private:
        static const std::string kClassName;
    };
}

#endif