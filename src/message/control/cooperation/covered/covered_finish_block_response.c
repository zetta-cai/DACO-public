#include "message/control/cooperation/covered/covered_finish_block_response.h"

namespace covered
{
    const std::string CoveredFinishBlockResponse::kClassName("CoveredFinishBlockResponse");

    CoveredFinishBlockResponse::CoveredFinishBlockResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredFinishBlockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    CoveredFinishBlockResponse::CoveredFinishBlockResponse(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    {
    }

    CoveredFinishBlockResponse::~CoveredFinishBlockResponse() {}
}