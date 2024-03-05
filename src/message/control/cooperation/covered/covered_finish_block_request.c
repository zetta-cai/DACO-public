#include "message/control/cooperation/covered/covered_finish_block_request.h"

namespace covered
{
    const std::string CoveredFinishBlockRequest::kClassName("CoveredFinishBlockRequest");

    CoveredFinishBlockRequest::CoveredFinishBlockRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredFinishBlockRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    CoveredFinishBlockRequest::CoveredFinishBlockRequest(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    {
    }

    CoveredFinishBlockRequest::~CoveredFinishBlockRequest() {}
}