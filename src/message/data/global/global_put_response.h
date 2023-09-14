/*
 * GlobalPutResponse: a response issued by cloud to an edge node for GlobalPutRequest.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_PUT_RESPONSE_H
#define GLOBAL_PUT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class GlobalPutResponse : public KeyMessage
    {
    public:
        GlobalPutResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        GlobalPutResponse(const DynamicArray& msg_payload);
        virtual ~GlobalPutResponse();
    private:
        static const std::string kClassName;
    };
}

#endif