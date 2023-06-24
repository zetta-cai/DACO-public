/*
 * GlobalGetRequest: a request issued by an edge node to cloud to get an existing value if any.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_GET_REQUEST_H
#define GLOBAL_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class GlobalGetRequest : public KeyMessage
    {
    public:
        GlobalGetRequest(const Key& key, const uint32_t& source_index);
        GlobalGetRequest(const DynamicArray& msg_payload);
        virtual ~GlobalGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif