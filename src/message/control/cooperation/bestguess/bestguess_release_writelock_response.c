#include "message/control/cooperation/bestguess/bestguess_release_writelock_response.h"

namespace covered
{
    const std::string BestGuessReleaseWritelockResponse::kClassName("BestGuessReleaseWritelockResponse");

    BestGuessReleaseWritelockResponse::BestGuessReleaseWritelockResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessReleaseWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessReleaseWritelockResponse::BestGuessReleaseWritelockResponse(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessReleaseWritelockResponse::~BestGuessReleaseWritelockResponse() {}
}