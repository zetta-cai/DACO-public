/*
 * BestGuessAcquireWritelockResponse: a response issued by beacon edge node to closest edge node to notify whether the closest edge node acquired permission for a write with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_ACQUIRE_WRITELOCK_RESPONSE_H
#define BESTGUESS_ACQUIRE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessAcquireWritelockResponse : public KeyByteSyncinfoMessage
    {
    public:
        BestGuessAcquireWritelockResponse(const Key& key, const LockResult& lock_result, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessAcquireWritelockResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessAcquireWritelockResponse();

        LockResult getLockResult() const;
    private:
        static const std::string kClassName;
    };
}

#endif