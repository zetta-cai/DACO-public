/*
 * CoveredFghybridHybridFetchedResponse: a foreground response issued by the beacon node to the closest node to acknowledge CoveredFghybridHybridFetchedRequest with victim synchronization for COVERED.
 *
 * NOTE: foreground response without placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.09).
 */

#ifndef COVERED_FGHYBRID_HYBRID_FETCHED_RESPONSE_H
#define COVERED_FGHYBRID_HYBRID_FETCHED_RESPONSE_H

#include <string>

#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredFghybridHybridFetchedResponse : public KeyVictimsetMessage
    {
    public:
        CoveredFghybridHybridFetchedResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredFghybridHybridFetchedResponse(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridHybridFetchedResponse();
    private:
        static const std::string kClassName;
    };
}

#endif