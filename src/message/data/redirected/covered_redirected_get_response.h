/*
 * CoveredRedirectedGetResponse: a response issued by the target edge node to an edge node for RedirectedGetRequest.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef COVERED_REDIRECTED_GET_RESPONSE_H
#define COVERED_REDIRECTED_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_hitflag_victimset_message.h"

namespace covered
{
    class CoveredRedirectedGetResponse : public KeyValueHitflagVictimsetMessage
    {
    public:
        CoveredRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredRedirectedGetResponse(const DynamicArray& msg_payload);
        virtual ~CoveredRedirectedGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif