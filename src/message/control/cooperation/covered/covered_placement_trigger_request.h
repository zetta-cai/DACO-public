/*
 * CoveredPlacementTriggerRequest: a request issued by an edge node to the beacon node to trigger trade-off-aware placement calculation of a given key with popularity collection and victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2024.02.05).
 */

#ifndef COVERED_PLACEMENT_TRIGGER_REQUEST_H
#define COVERED_PLACEMENT_TRIGGER_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_collectpop_victimset_message.h"

namespace covered
{
    class CoveredPlacementTriggerRequest : public KeyCollectpopVictimsetMessage
    {
    public:
        CoveredPlacementTriggerRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementTriggerRequest(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementTriggerRequest();
    private:
        static const std::string kClassName;
    };
}

#endif