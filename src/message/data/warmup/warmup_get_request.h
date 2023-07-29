/*
 * WarmupGetRequest: a request issued by a client to the closest edge node to get an existing value if any under speedup mode for warmup phase.
 * 
 * By Siyuan Sheng (2023.07.29).
 */

#ifndef WARMUP_GET_REQUEST_H
#define WARMUP_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class WarmupGetRequest : public KeyMessage
    {
    public:
        WarmupGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr);
        WarmupGetRequest(const DynamicArray& msg_payload);
        virtual ~WarmupGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif