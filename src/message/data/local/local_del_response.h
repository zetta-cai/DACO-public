/*
 * LocalDelResponse: a response issued by the closest edge node to a client for LocalDelRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_DEL_RESPONSE_H
#define LOCAL_DEL_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_hitflag_utilization_message.h"

namespace covered
{
    class LocalDelResponse : public KeyHitflagUtilizationMessage
    {
    public:
        LocalDelResponse(const Key& key, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        LocalDelResponse(const DynamicArray& msg_payload);
        virtual ~LocalDelResponse();

        //Hitflag getHitflag() const;
    private:
        static const std::string kClassName;
    };
}

#endif