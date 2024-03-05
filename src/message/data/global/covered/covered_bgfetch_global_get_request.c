#include "message/data/global/covered/covered_bgfetch_global_get_request.h"

namespace covered
{
    const std::string CoveredBgfetchGlobalGetRequest::kClassName("CoveredBgfetchGlobalGetRequest");

    CoveredBgfetchGlobalGetRequest::CoveredBgfetchGlobalGetRequest(const Key& key, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyEdgesetMessage(key, edgeset, MessageType::kCoveredBgfetchGlobalGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    CoveredBgfetchGlobalGetRequest::CoveredBgfetchGlobalGetRequest(const DynamicArray& msg_payload) : KeyEdgesetMessage(msg_payload)
    {
    }

    CoveredBgfetchGlobalGetRequest::~CoveredBgfetchGlobalGetRequest() {}
}