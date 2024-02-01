/*
 * CoveredAcquireWritelockResponse: a response issued by beacon edge node to closest edge node to notify whether the closest edge node acquired permission for a write with victim synchroization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.09).
 */

#ifndef COVERED_ACQUIRE_WRITELOCK_RESPONSE_H
#define COVERED_ACQUIRE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_victimset_message.h"

namespace covered
{
    class CoveredAcquireWritelockResponse : public KeyByteVictimsetMessage
    {
    public:
        CoveredAcquireWritelockResponse(const Key& key, const LockResult& lock_result, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredAcquireWritelockResponse(const DynamicArray& msg_payload);
        virtual ~CoveredAcquireWritelockResponse();

        LockResult getLockResult() const;
    private:
        static const std::string kClassName;
    };
}

#endif