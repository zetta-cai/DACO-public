#include "message/control/acquire_writelock_response.h"

namespace covered
{
    const std::string AcquireWritelockResponse::kClassName("AcquireWritelockResponse");

    AcquireWritelockResponse::AcquireWritelockResponse(const Key& key, const LockResult& lock_result, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyByteMessage(key, static_cast<uint8_t>(lock_result), MessageType::kAcquireWritelockResponse, source_index, source_addr, event_list)
    {
    }

    AcquireWritelockResponse::AcquireWritelockResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    AcquireWritelockResponse::~AcquireWritelockResponse() {}

    LockResult AcquireWritelockResponse::getLockResult() const
    {
        uint8_t byte = getByte_();
        LockResult lock_result = static_cast<LockResult>(byte);
        return lock_result;
    }
}