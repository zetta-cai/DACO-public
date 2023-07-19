/*
 * InvalidationResponse: a response issued by each involved edge node to closest edge node or beacon edge node to acknowledge InvalidationResponse.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef INVALIDATION_RESPONSE_H
#define INVALIDATION_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class InvalidationResponse : public KeyMessage
    {
    public:
        InvalidationResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        InvalidationResponse(const DynamicArray& msg_payload);
        virtual ~InvalidationResponse();
    private:
        static const std::string kClassName;
    };
}

#endif