#include "message/control/cooperation/covered/covered_fghybrid_release_writelock_response.h"

namespace covered
{
    const std::string CoveredFghybridReleaseWritelockResponse::kClassName("CoveredFghybridReleaseWritelockResponse");

    CoveredFghybridReleaseWritelockResponse::CoveredFghybridReleaseWritelockResponse(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetEdgesetMessage(key, victim_syncset, edgeset, MessageType::kCoveredFghybridReleaseWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    CoveredFghybridReleaseWritelockResponse::CoveredFghybridReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridReleaseWritelockResponse::~CoveredFghybridReleaseWritelockResponse() {}
}