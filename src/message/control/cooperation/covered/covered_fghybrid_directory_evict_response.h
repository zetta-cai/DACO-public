/*
 * CoveredFghybridDirectoryEvictResponse: a response issued by the beacon node to the closest node to acknowledge CoveredDirectoryUpdateRequest with victim synchronization for COVERED; also trigger hybrid data fetching for non-blocking placement notification later.
 *
 * NOTE: foreground response with placement edgeset to trigger non-blocking placement notification.
 * 
 * NOTE: CoveredFghybridDirectoryEvictResponse is foreground and ONLY used for hybrid data fetching under non-blocking placement deployment, while CoveredBgplaceDirectoryUpdateResponse is background and ONLY used for background events and bandwidth usage, yet NOT related with hybrid data fetching under non-blocking placement deployment.
 * 
 * By Siyuan Sheng (2023.10.11).
 */

#ifndef COVERED_FGHYBRID_DIRECTORY_EVICT_RESPONSE_H
#define COVERED_FGHYBRID_DIRECTORY_EVICT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_victimset_edgeset_message.h"

namespace covered
{
    class CoveredFghybridDirectoryEvictResponse : public KeyByteVictimsetEdgesetMessage
    {
    public:
        CoveredFghybridDirectoryEvictResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredFghybridDirectoryEvictResponse(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridDirectoryEvictResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif