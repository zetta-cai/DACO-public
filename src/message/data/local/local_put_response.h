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
#include "message/key_hitflag_utilization_message.h"

namespace covered
{
    class LocalPutResponse : public KeyHitflagUtilizationMessage
    {
    public:
        LocalPutResponse(const Key& key, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        LocalPutResponse(const DynamicArray& msg_payload);
        virtual ~LocalPutResponse();

        //Hitflag getHitflag() const;
    private:
        static const std::string kClassName;
    };
}

#endif