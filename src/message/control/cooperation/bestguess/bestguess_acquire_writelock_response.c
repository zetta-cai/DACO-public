#include "message/control/cooperation/bestguess/bestguess_acquire_writelock_response.h"

namespace covered
{
    const std::string BestGuessAcquireWritelockResponse::kClassName("BestGuessAcquireWritelockResponse");

    BestGuessAcquireWritelockResponse::BestGuessAcquireWritelockResponse(const Key& key, const LockResult& lock_result, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyByteSyncinfoMessage(key, static_cast<uint8_t>(lock_result), syncinfo, MessageType::kBestGuessAcquireWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessAcquireWritelockResponse::BestGuessAcquireWritelockResponse(const DynamicArray& msg_payload) : KeyByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessAcquireWritelockResponse::~BestGuessAcquireWritelockResponse() {}

    LockResult BestGuessAcquireWritelockResponse::getLockResult() const
    {
        uint8_t byte = getByte_();
        LockResult lock_result = static_cast<LockResult>(byte);
        return lock_result;
    }
}