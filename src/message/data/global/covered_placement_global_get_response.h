/*
 * CoveredPlacementGlobalGetResponse: a response issued by cloud to an edge node for GlobalGetRequest.
 * 
 * NOTE: background response with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef COVERED_PLACEMENT_GLOBAL_GET_RESPONSE_H
#define COVERED_PLACEMENT_GLOBAL_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_edgeset_message.h"

namespace covered
{
    class CoveredPlacementGlobalGetResponse : public KeyValueEdgesetMessage
    {
    public:
        CoveredPlacementGlobalGetResponse(const Key& key, const Value& value, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredPlacementGlobalGetResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementGlobalGetResponse();
    private:
        static const std::string kClassName;
    };
}

#endif