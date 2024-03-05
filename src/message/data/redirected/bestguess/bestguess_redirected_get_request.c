#include "message/data/redirected/bestguess/bestguess_redirected_get_request.h"

namespace covered
{
    const std::string BestGuessRedirectedGetRequest::kClassName("BestGuessRedirectedGetRequest");

    BestGuessRedirectedGetRequest::BestGuessRedirectedGetRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessRedirectedGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessRedirectedGetRequest::BestGuessRedirectedGetRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessRedirectedGetRequest::~BestGuessRedirectedGetRequest() {}
}