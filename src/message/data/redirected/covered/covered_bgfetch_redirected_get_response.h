/*
 * CoveredBgfetchRedirectedGetResponse: a response issued by the target edge node to an edge node for CoveredBgfetchRedirectedGetRequest.
 * 
 * NOTE: background response with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.09.25).
 */

#ifndef COVERED_BGFETCH_REDIRECTED_GET_RESPONSE_H
#define COVERED_BGFETCH_REDIRECTED_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_hitflag_victimset_edgeset_message.h"

namespace covered
{
    class CoveredBgfetchRedirectedGetResponse : public KeyValueHitflagVictimsetEdgesetMessage
    {
    public:
        CoveredBgfetchRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredBgfetchRedirectedGetResponse(const DynamicArray& msg_payload);
        virtual ~CoveredBgfetchRedirectedGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif