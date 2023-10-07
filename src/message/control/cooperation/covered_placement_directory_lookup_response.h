/*
 * CoveredPlacementDirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key with victim synchronization for COVERED; also with hybrid data fetching to trigger non-blocking placement notification.
 * 
 * NOTE: foreground response with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.07).
 */

#ifndef COVERED_PLACEMENT_DIRECTORY_LOOKUP_RESPONSE_H
#define COVERED_PLACEMENT_DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_writeflag_validity_directory_victimset_edgeset_message.h"

namespace covered
{
    class CoveredPlacementDirectoryLookupResponse : public KeyWriteflagValidityDirectoryVictimsetEdgesetMessage
    {
    public:
        CoveredPlacementDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredPlacementDirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementDirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif