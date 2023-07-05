/*
 * LocalGetRequest: a request issued by a client to the closest edge node to get an existing value if any.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_GET_REQUEST_H
#define LOCAL_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class LocalGetRequest : public KeyMessage
    {
    public:
        LocalGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr);
        LocalGetRequest(const DynamicArray& msg_payload);
        virtual ~LocalGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif