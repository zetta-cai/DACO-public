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
#include "message/key_hitflag_message.h"

namespace covered
{
    class LocalDelResponse : public KeyHitflagMessage
    {
    public:
        LocalDelResponse(const Key& key, const Hitflag& hitflag);
        LocalDelResponse(const DynamicArray& msg_payload);
        virtual ~LocalDelResponse();
    private:
        static const std::string kClassName;
    };
}

#endif