/*
 * AcquireWritelockResponse: a response issued by beacon edge node to closest edge node to notify whether the closest edge node acquired permission for a write.
 * 
 * By Siyuan Sheng (2023.06.22).
 */

#ifndef ACQUIRE_WRITELOCK_RESPONSE_H
#define ACQUIRE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_message.h"

namespace covered
{
    class AcquireWritelockResponse : public KeyByteMessage
    {
    public:
        AcquireWritelockResponse(const Key& key, const LockResult& lock_result, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        AcquireWritelockResponse(const DynamicArray& msg_payload);
        virtual ~AcquireWritelockResponse();

        LockResult getLockResult() const;
    private:
        static const std::string kClassName;
    };
}

#endif