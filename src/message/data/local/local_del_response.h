/*
 * LocalDelResponse: a response issued by the closest edge node to a client for LocalDelRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_DEL_RESPONSE_H
#define LOCAL_DEL_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class LocalDelResponse : public KeyMessage
    {
    public:
        LocalDelResponse(const Key& key);
        LocalDelResponse(const DynamicArray& msg_payload);
        ~LocalDelResponse();
    private:
        static const std::string kClassName;
    };
}

#endif