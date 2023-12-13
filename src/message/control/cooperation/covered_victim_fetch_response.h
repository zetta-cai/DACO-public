/*
 * CoveredVictimFetchResponse: a response issued by an edge node to to acknowledge CoveredVictimFetchResponse for lazy victim fetching with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.10.05).
 */

#ifndef COVERED_VICTIM_FETCH_RESPONSE_H
#define COVERED_VICTIM_FETCH_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
//#include "message/double_victimset_message.h"
#include "message/single_victimset_message.h"

namespace covered
{
    // class CoveredVictimFetchResponse : public DoubleVictimsetMessage
    class CoveredVictimFetchResponse : public SingleVictimsetMessage
    {
    public:
        // CoveredVictimFetchResponse(const VictimSyncset& victim_fetchset, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredVictimFetchResponse(const VictimSyncset& victim_fetchset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredVictimFetchResponse(const DynamicArray& msg_payload);
        virtual ~CoveredVictimFetchResponse();
    private:
        static const std::string kClassName;
    };
}

#endif