/*
 * AcquireWritelockResponse: a response issued by beacon edge node to closest edge node to notify whether the closest edge node acquired permission for a write.
 * 
 * By Siyuan Sheng (2023.06.22).
 */

#ifndef ACQUIRE_WRITELOCK_RESPONSE_H
#define ACQUIRE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class AcquireWritelockResponse : public KeyMessage
    {
    public:
        AcquireWritelockResponse(const Key& key);
        AcquireWritelockResponse(const DynamicArray& msg_payload);
        virtual ~AcquireWritelockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif