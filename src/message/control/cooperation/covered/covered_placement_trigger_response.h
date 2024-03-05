/*
 * CoveredPlacementTriggerResponse: a response issued by the beacon node to an edge node to reply placement trigger request of a given key with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2024.02.05).
 */

#ifndef COVERED_PLACEMENT_TRIGGER_RESPONSE_H
#define COVERED_PLACEMENT_TRIGGER_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredPlacementTriggerResponse : public KeyVictimsetMessage
    {
    public:
        CoveredPlacementTriggerResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredPlacementTriggerResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementTriggerResponse();
    private:
        static const std::string kClassName;
    };
}

#endif