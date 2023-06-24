/*
 * InvalidationRequest: a request issued by closest edge node or beacon edge node to neighbors to invalidate all cache copies for MSI protocol.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef INVALIDATION_REQUEST_H
#define INVALIDATION_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class InvalidationRequest : public KeyMessage
    {
    public:
        InvalidationRequest(const Key& key, const uint32_t& source_index);
        InvalidationRequest(const DynamicArray& msg_payload);
        virtual ~InvalidationRequest();
    private:
        static const std::string kClassName;
    };
}

#endif