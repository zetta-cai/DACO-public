/*
 * RedirectedGetResponse: a response issued by the target edge node to an edge node for RedirectedGetRequest.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef REDIRECTED_GET_RESPONSE_H
#define REDIRECTED_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_hitflag_message.h"

namespace covered
{
    class RedirectedGetResponse : public KeyValueHitflagMessage
    {
    public:
        RedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        RedirectedGetResponse(const DynamicArray& msg_payload);
        virtual ~RedirectedGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif