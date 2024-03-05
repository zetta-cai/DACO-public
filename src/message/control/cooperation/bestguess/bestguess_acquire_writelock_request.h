/*
 * BestGuessAcquireWritelockRequest: a request issued by closest edge node to the beacon node to acquire permission for a write with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_ACQUIRE_WRITELOCK_REQUEST_H
#define BESTGUESS_ACQUIRE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessAcquireWritelockRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessAcquireWritelockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessAcquireWritelockRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessAcquireWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif