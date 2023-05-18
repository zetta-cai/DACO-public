/*
 * LocalGetResponse: a response issued by the closest edge node to a client for LocalGetRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_GET_RESPONSE_H
#define LOCAL_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_message.h"

namespace covered
{
    class LocalGetResponse : public KeyValueMessage
    {
    public:
        LocalGetResponse(const Key& key, const Value& value);
        LocalGetResponse(const DynamicArray& msg_payload);
        ~LocalGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif