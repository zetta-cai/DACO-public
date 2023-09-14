/*
 * CoveredReleaseWritelockResponse: a response issued by beacon edge node to acknowledge CoveredReleaseWritelockRequest with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.09).
 */

#ifndef COVERED_RELEASE_WRITELOCK_RESPONSE_H
#define COVERED_RELEASE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredReleaseWritelockResponse : public KeyVictimsetMessage
    {
    public:
        CoveredReleaseWritelockResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredReleaseWritelockResponse(const DynamicArray& msg_payload);
        virtual ~CoveredReleaseWritelockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif