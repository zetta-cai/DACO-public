/*
 * LocalPutResponse: a response issued by the closest edge node to a client for LocalPutRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_PUT_RESPONSE_H
#define LOCAL_PUT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_hitflag_message.h"

namespace covered
{
    class LocalPutResponse : public KeyHitflagMessage
    {
    public:
        LocalPutResponse(const Key& key, const Hitflag& hitflag);
        LocalPutResponse(const DynamicArray& msg_payload);
        ~LocalPutResponse();
    private:
        static const std::string kClassName;
    };
}

#endif