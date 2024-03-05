#include "message/control/cooperation/covered/covered_acquire_writelock_response.h"

namespace covered
{
    const std::string CoveredAcquireWritelockResponse::kClassName("CoveredAcquireWritelockResponse");

    CoveredAcquireWritelockResponse::CoveredAcquireWritelockResponse(const Key& key, const LockResult& lock_result, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyByteVictimsetMessage(key, static_cast<uint8_t>(lock_result), victim_syncset, MessageType::kCoveredAcquireWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    CoveredAcquireWritelockResponse::CoveredAcquireWritelockResponse(const DynamicArray& msg_payload) : KeyByteVictimsetMessage(msg_payload)
    {
    }

    CoveredAcquireWritelockResponse::~CoveredAcquireWritelockResponse() {}

    LockResult CoveredAcquireWritelockResponse::getLockResult() const
    {
        uint8_t byte = getByte_();
        LockResult lock_result = static_cast<LockResult>(byte);
        return lock_result;
    }
}