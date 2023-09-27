/*
 * CoveredPlacementCoveredPlacementGlobalGetRequest: a request issued by an edge node to cloud to get an existing value if any for COVERED's non-blocking placement deployment.
 * 
 * By Siyuan Sheng (2023.09.27).
 */

#ifndef COVERED_PLACEMENT_GLOBAL_GET_REQUEST_H
#define COVERED_PLACEMENT_GLOBAL_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_edgeset_message.h"

namespace covered
{
    class CoveredPlacementGlobalGetRequest : public KeyEdgesetMessage
    {
    public:
        CoveredPlacementGlobalGetRequest(const Key& key, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementGlobalGetRequest(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementGlobalGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif