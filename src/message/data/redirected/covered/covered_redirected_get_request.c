#include "message/data/redirected/covered/covered_redirected_get_request.h"

namespace covered
{
    const std::string CoveredRedirectedGetRequest::kClassName("CoveredRedirectedGetRequest");

    CoveredRedirectedGetRequest::CoveredRedirectedGetRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredRedirectedGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    CoveredRedirectedGetRequest::CoveredRedirectedGetRequest(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    {
    }

    CoveredRedirectedGetRequest::~CoveredRedirectedGetRequest() {}
}