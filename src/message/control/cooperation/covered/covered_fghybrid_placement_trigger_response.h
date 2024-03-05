/*
 * CoveredFghybridPlacementTriggerResponse: a foreground response issued by the beacon node to an edge node to reply placement trigger request of a given key to trigger hybrid fetching with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2024.02.05).
 */

#ifndef COVERED_FGHYBRID_PLACEMENT_TRIGGER_RESPONSE_H
#define COVERED_FGHYBRID_PLACEMENT_TRIGGER_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_edgeset_message.h"

namespace covered
{
    class CoveredFghybridPlacementTriggerResponse : public KeyVictimsetEdgesetMessage
    {
    public:
        CoveredFghybridPlacementTriggerResponse(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredFghybridPlacementTriggerResponse(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridPlacementTriggerResponse();
    private:
        static const std::string kClassName;
    };
}

#endif