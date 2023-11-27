/*
 * CoveredPlacementDirectoryAdmitResponse: a response issued by the beacon node to the closest node to acknowledge CoveredPlacementDirectoryAdmitRequest with victim synchronization for COVERED.
 *
 * NOTE: foreground response without placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.10).
 */

#ifndef COVERED_PLACEMENT_DIRECTORY_ADMIT_RESPONSE_H
#define COVERED_PLACEMENT_DIRECTORY_ADMIT_RESPONSE_H

#include <string>

#include "message/key_two_byte_victimset_message.h"

namespace covered
{
    class CoveredPlacementDirectoryAdmitResponse : public KeyTwoByteVictimsetMessage
    {
    public:
        CoveredPlacementDirectoryAdmitResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredPlacementDirectoryAdmitResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementDirectoryAdmitResponse();

        bool isBeingWritten() const;
        bool isNeighborCached() const;
    private:
        static const std::string kClassName;
    };
}

#endif