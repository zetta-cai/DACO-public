/*
 * CoveredPlacementDirectoryUpdateRequest: a request issued by the closest node to the beacon node to update directory information of a given key with popularity collection (if key is evicted) and victim synchronization for COVERED during non-blocking placement deployment.
 *
 * NOTE: background request without placement edgeset to trigger non-blocking placement notification.
 *
 * NOTE: is_valid_directory_exist indicates whether to add or delete the directory information.
 * 
 * NOTE: the collected popularity will be serialized/deserialized ONLY if validity = false (i.e., is_admit = false), where collected_popularity_.isTracked() indicates that whether the evicted key is selected for metadata preservation.
 * 
 * NOTE: CoveredPlacementDirectoryUpdateRequest does NOT need placement edgeset.
 * 
 * NOTE: CoveredPlacementDirectoryUpdateRequest with is_admit = true is equivalent to ACK for CoveredPlacementNotifyRequest.
 * 
 * By Siyuan Sheng (2023.10.02).
 */

#ifndef COVERED_PLACEMENT_DIRECTORY_UPDATE_REQUEST_H
#define COVERED_PLACEMENT_DIRECTORY_UPDATE_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_validity_directory_collectpop_victimset_message.h"

namespace covered
{
    class CoveredPlacementDirectoryUpdateRequest : public KeyValidityDirectoryCollectpopVictimsetMessage
    {
    public:
        CoveredPlacementDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementDirectoryUpdateRequest(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementDirectoryUpdateRequest();
    private:
        static const std::string kClassName;
    };
}

#endif