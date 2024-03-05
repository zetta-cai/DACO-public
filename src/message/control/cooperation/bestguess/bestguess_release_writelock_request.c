#include "message/control/cooperation/bestguess/bestguess_release_writelock_request.h"

namespace covered
{
    const std::string BestGuessReleaseWritelockRequest::kClassName("BestGuessReleaseWritelockRequest");

    BestGuessReleaseWritelockRequest::BestGuessReleaseWritelockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessReleaseWritelockRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessReleaseWritelockRequest::BestGuessReleaseWritelockRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessReleaseWritelockRequest::~BestGuessReleaseWritelockRequest() {}
}