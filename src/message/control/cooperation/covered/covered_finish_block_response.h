/*
 * CoveredCoveredFinishBlockResponse: a response issued by a closest edge node to acknowledge CoveredFinishBlockRequest from the beacon node with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.10.21).
 */

#ifndef COVERED_FINISH_BLOCK_RESPONSE_H
#define COVERED_FINISH_BLOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredFinishBlockResponse : public KeyVictimsetMessage
    {
    public:
        CoveredFinishBlockResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredFinishBlockResponse(const DynamicArray& msg_payload);
        virtual ~CoveredFinishBlockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif