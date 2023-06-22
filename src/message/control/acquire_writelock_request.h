/*
 * AcquireWritelockRequest: a request issued by closest edge node to the beacon node to acquire permission for a write.
 * 
 * By Siyuan Sheng (2023.06.22).
 */

#ifndef ACQUIRE_WRITELOCK_REQUEST_H
#define ACQUIRE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class AcquireWritelockRequest : public KeyMessage
    {
    public:
        AcquireWritelockRequest(const Key& key);
        AcquireWritelockRequest(const DynamicArray& msg_payload);
        virtual ~AcquireWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif