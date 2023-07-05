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
#include "message/key_byte_message.h"

namespace covered
{
    class LocalDelResponse : public KeyByteMessage
    {
    public:
        LocalDelResponse(const Key& key, const Hitflag& hitflag, const uint32_t& source_index, const NetworkAddr& source_addr);
        LocalDelResponse(const DynamicArray& msg_payload);
        virtual ~LocalDelResponse();

        Hitflag getHitflag() const;
    private:
        static const std::string kClassName;
    };
}

#endif