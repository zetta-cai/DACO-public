/*
 * GlobalGetResponse: a response issued by cloud to an edge node for GlobalGetRequest.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_GET_RESPONSE_H
#define GLOBAL_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_message.h"

namespace covered
{
    class GlobalGetResponse : public KeyValueMessage
    {
    public:
        GlobalGetResponse(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        GlobalGetResponse(const DynamicArray& msg_payload);
        virtual ~GlobalGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif