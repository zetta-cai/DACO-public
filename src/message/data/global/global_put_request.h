/*
 * GlobalPutRequest: a request issued by an edge node to cloud to insert/update a new value.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_PUT_REQUEST_H
#define GLOBAL_PUT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_message.h"

namespace covered
{
    class GlobalPutRequest : public KeyValueMessage
    {
    public:
        GlobalPutRequest(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr);
        GlobalPutRequest(const DynamicArray& msg_payload);
        virtual ~GlobalPutRequest();
    private:
        static const std::string kClassName;
    };
}

#endif