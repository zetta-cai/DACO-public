/*
 * LocalPutRequest: a request issued by a client to the closest edge node to insert/update a new value.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_PUT_REQUEST_H
#define LOCAL_PUT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_message.h"

namespace covered
{
    class LocalPutRequest : public KeyValueMessage
    {
    public:
        LocalPutRequest(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        LocalPutRequest(const DynamicArray& msg_payload);
        virtual ~LocalPutRequest();
    private:
        static const std::string kClassName;
    };
}

#endif