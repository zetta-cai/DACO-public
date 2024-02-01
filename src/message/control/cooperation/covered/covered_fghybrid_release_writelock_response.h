/*
 * CoveredFghybridReleaseWritelockResponse: a response issued by beacon edge node to acknowledge CoveredReleaseWritelockRequest with victim synchronization for COVERED; also trigger hybrid data fetching for non-blocking placement notification.
 * 
 * NOTE: foreground response with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.11).
 */

#ifndef COVERED_FGHYBRID_RELEASE_WRITELOCK_RESPONSE_H
#define COVERED_FGHYBRID_RELEASE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_edgeset_message.h"

namespace covered
{
    class CoveredFghybridReleaseWritelockResponse : public KeyVictimsetEdgesetMessage
    {
    public:
        CoveredFghybridReleaseWritelockResponse(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredFghybridReleaseWritelockResponse(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridReleaseWritelockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif