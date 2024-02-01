/*
 * CoveredBgplaceDirectoryUpdateResponse: a response issued by the beacon node to the closest node to acknowledge CoveredBgplaceDirectoryUpdateRequest with victim synchronization for COVERED during non-blocking placement deployment.
 *
 * NOTE: background response without placement edgeset to trigger non-blocking placement notification.
 * 
 * NOTE: CoveredBgplaceDirectoryUpdateResponse is background and ONLY used for background events and bandwidth usage, yet NOT related with hybrid data fetching under non-blocking placement deployment, while CoveredFghybridDirectoryEvictResponse is foreground and ONLY used for hybrid data fetching under non-blocking placement deployment.
 * 
 * By Siyuan Sheng (2023.10.02).
 */

#ifndef COVERED_BGPLACE_DIRECTORY_UPDATE_RESPONSE_H
#define COVERED_BGPLACE_DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_two_byte_victimset_message.h"

namespace covered
{
    class CoveredBgplaceDirectoryUpdateResponse : public KeyTwoByteVictimsetMessage
    {
    public:
        CoveredBgplaceDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredBgplaceDirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~CoveredBgplaceDirectoryUpdateResponse();

        bool isBeingWritten() const;
        bool isNeighborCached() const;
    private:
        static const std::string kClassName;
    };
}

#endif