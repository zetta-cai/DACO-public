/*
 * CoveredInvalidationResponse: a response issued by each involved edge node to closest edge node or beacon edge node to acknowledge CoveredInvalidationResponse with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.10.21).
 */

#ifndef COVERED_INVALIDATION_RESPONSE_H
#define COVERED_INVALIDATION_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredInvalidationResponse : public KeyVictimsetMessage
    {
    public:
        CoveredInvalidationResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredInvalidationResponse(const DynamicArray& msg_payload);
        virtual ~CoveredInvalidationResponse();
    private:
        static const std::string kClassName;
    };
}

#endif