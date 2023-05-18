/*
 * GlobalDelRequest: a request issued by an edge node to cloud to delete an existing value if any.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef GLOBAL_DEL_REQUEST_H
#define GLOBAL_DEL_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class GlobalDelRequest : public KeyMessage
    {
    public:
        GlobalDelRequest(const Key& key);
        GlobalDelRequest(const DynamicArray& msg_payload);
        ~GlobalDelRequest();
    private:
        static const std::string kClassName;
    };
}

#endif