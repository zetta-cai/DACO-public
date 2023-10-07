/*
 * CoveredPlacementRedirectedGetRequest: a request issued by an edge node to the target edge node to get an existing value if any with victim synchronization and placement edgeset for non-blocking placement deployment in COVERED.
 * 
 * NOTE: background request with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.09.25).
 */

#ifndef COVERED_PLACEMENT_REDIRECTED_GET_REQUEST_H
#define COVERED_PLACEMENT_REDIRECTED_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_edgeset_message.h"

namespace covered
{
    class CoveredPlacementRedirectedGetRequest : public KeyVictimsetEdgesetMessage
    {
    public:
        CoveredPlacementRedirectedGetRequest(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementRedirectedGetRequest(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementRedirectedGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif