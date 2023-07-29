/*
 * WarmupPutRequest: a request issued by a client to the closest edge node to insert/update a new value under speedup mode for warmup phase.
 * 
 * By Siyuan Sheng (2023.07.29).
 */

#ifndef WARMUP_PUT_REQUEST_H
#define WARMUP_PUT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_message.h"

namespace covered
{
    class WarmupPutRequest : public KeyValueMessage
    {
    public:
        WarmupPutRequest(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr);
        WarmupPutRequest(const DynamicArray& msg_payload);
        virtual ~WarmupPutRequest();
    private:
        static const std::string kClassName;
    };
}

#endif