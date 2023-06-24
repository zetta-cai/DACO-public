/*
 * ReleaseWritelockResponse: a response issued by beacon edge node to acknowledge ReleaseWritelockRequest.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef RELEASE_WRITELOCK_RESPONSE_H
#define RELEASE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class ReleaseWritelockResponse : public KeyMessage
    {
    public:
        ReleaseWritelockResponse(const Key& key, const uint32_t& source_index);
        ReleaseWritelockResponse(const DynamicArray& msg_payload);
        virtual ~ReleaseWritelockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif