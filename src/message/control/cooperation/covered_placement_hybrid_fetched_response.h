/*
 * CoveredPlacementHybridFetchedResponse: a response issued by the beacon node to the closest node to acknowledge CoveredPlacementHybridFetchedRequest with victim synchronization for COVERED.
 *
 * NOTE: foreground response without placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.09).
 */

#ifndef COVERED_PLACEMENT_HYBRID_FETCHED_RESPONSE_H
#define COVERED_PLACEMENT_HYBRID_FETCHED_RESPONSE_H

#include <string>

#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredPlacementHybridFetchedResponse : public KeyVictimsetMessage
    {
    public:
        CoveredPlacementHybridFetchedResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredPlacementHybridFetchedResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementHybridFetchedResponse();
    private:
        static const std::string kClassName;
    };
}

#endif