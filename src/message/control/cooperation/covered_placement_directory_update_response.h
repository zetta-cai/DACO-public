/*
 * CoveredPlacementDirectoryUpdateResponse: a response issued by the beacon node to the closest node to acknowledge CoveredPlacementDirectoryUpdateRequest with victim synchronization for COVERED during non-blocking placement deployment.
 *
 * NOTE: background response without placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.02).
 */

#ifndef COVERED_PLACEMENT_DIRECTORY_UPDATE_RESPONSE_H
#define COVERED_PLACEMENT_DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_victimset_message.h"

namespace covered
{
    class CoveredPlacementDirectoryUpdateResponse : public KeyByteVictimsetMessage
    {
    public:
        CoveredPlacementDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredPlacementDirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementDirectoryUpdateResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif