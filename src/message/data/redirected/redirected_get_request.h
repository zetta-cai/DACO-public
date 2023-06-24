/*
 * RedirectedGetRequest: a request issued by an edge node to the target edge node to get an existing value if any.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef REDIRECTED_GET_REQUEST_H
#define REDIRECTED_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class RedirectedGetRequest : public KeyMessage
    {
    public:
        RedirectedGetRequest(const Key& key, const uint32_t& source_index);
        RedirectedGetRequest(const DynamicArray& msg_payload);
        virtual ~RedirectedGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif