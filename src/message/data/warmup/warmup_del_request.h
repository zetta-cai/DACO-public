/*
 * WarmupDelRequest: a request issued by a client to the closest edge node to delete an existing value if any under speedup mode for warmup phase.
 * 
 * By Siyuan Sheng (2023.07.29).
 */

#ifndef WARMUP_DEL_REQUEST_H
#define WARMUP_DEL_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class WarmupDelRequest : public KeyMessage
    {
    public:
        WarmupDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr);
        WarmupDelRequest(const DynamicArray& msg_payload);
        virtual ~WarmupDelRequest();
    private:
        static const std::string kClassName;
    };
}

#endif