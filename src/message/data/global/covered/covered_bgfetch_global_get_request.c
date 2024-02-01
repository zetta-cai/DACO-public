#include "message/data/global/covered/covered_bgfetch_global_get_request.h"

namespace covered
{
    const std::string CoveredBgfetchGlobalGetRequest::kClassName("CoveredBgfetchGlobalGetRequest");

    CoveredBgfetchGlobalGetRequest::CoveredBgfetchGlobalGetRequest(const Key& key, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyEdgesetMessage(key, edgeset, MessageType::kCoveredPlacementGlobalGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredBgfetchGlobalGetRequest::CoveredBgfetchGlobalGetRequest(const DynamicArray& msg_payload) : KeyEdgesetMessage(msg_payload)
    {
    }

    CoveredBgfetchGlobalGetRequest::~CoveredBgfetchGlobalGetRequest() {}
}