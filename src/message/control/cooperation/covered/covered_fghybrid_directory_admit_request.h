/*
 * CoveredFghybridDirectoryAdmitRequest: a foreground request issued by the closest node to the beacon node to add directory information of a newly-admitted key, perform victim synchronization, and notify the result of including-sender hybrid data fetching to trigger COVERED's non-blocking placement notification.
 *
 * NOTE: foreground request with placement edgeset to trigger non-blocking placement notification.
 * 
 * NOTE: CoveredFghybridDirectoryAdmitRequest is a foreground message and ONLY used for hybrid data fetching to trigger non-blocking placement notification; while CoveredBgplaceDirectoryUpdateRequest is ONLY used for background events and bandwidth usage, yet NOT related with hybrid data fetching.
 * 
 * By Siyuan Sheng (2023.10.07).
 */

#ifndef COVERED_FGHYBRID_DIRECTORY_ADMIT_REQUEST_H
#define COVERED_FGHYBRID_DIRECTORY_ADMIT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_value_directory_victimset_edgeset_message.h"

namespace covered
{
    class CoveredFghybridDirectoryAdmitRequest : public KeyValueDirectoryVictimsetEdgesetMessage
    {
    public:
        CoveredFghybridDirectoryAdmitRequest(const Key& key, const Value& value, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredFghybridDirectoryAdmitRequest(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridDirectoryAdmitRequest();
    private:
        static const std::string kClassName;
    };
}

#endif