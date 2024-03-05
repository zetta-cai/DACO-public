/*
 * LocalDelRequest: a request issued by a client to the closest edge node to delete an existing value if any.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_DEL_REQUEST_H
#define LOCAL_DEL_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class LocalDelRequest : public KeyMessage
    {
    public:
        LocalDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        LocalDelRequest(const DynamicArray& msg_payload);
        virtual ~LocalDelRequest();
    private:
        static const std::string kClassName;
    };
}

#endif