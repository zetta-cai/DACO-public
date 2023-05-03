/*
 * Request: a general request to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class Request
    {
    public:
        Request(const Key& key, const Value& value);
        ~Request();

        Key getKey();
        Value getValue();

        uint32_t getMsgPayloadSize();

        // Offset of request must be 0 in message payload
        uint32_t serialize(DynamicArray& msg_payload);
        uint32_t deserialize(const DynamicArray& msg_payload);
    private:
        static const std::string kClassName;

        Key key_;
        Value value_;
    };
}

#endif