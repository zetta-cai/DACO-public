/*
 * LocalGetResponse: a response issued by the closest edge node to a client for LocalGetRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_GET_RESPONSE_H
#define LOCAL_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_hitflag_utilization_message.h"

namespace covered
{
    class LocalGetResponse : public KeyValueHitflagUtilizationMessage
    {
    public:
        LocalGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        LocalGetResponse(const DynamicArray& msg_payload);
        virtual ~LocalGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif