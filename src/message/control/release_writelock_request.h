/*
 * ReleaseWritelockRequest: a request issued by closest edge node to beacon to finish writes.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef RELEASE_WRITELOCK_REQUEST_H
#define RELEASE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class ReleaseWritelockRequest : public KeyMessage
    {
    public:
        ReleaseWritelockRequest(const Key& key, const uint32_t& source_index);
        ReleaseWritelockRequest(const DynamicArray& msg_payload);
        virtual ~ReleaseWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif