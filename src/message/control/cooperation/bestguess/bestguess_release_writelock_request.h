/*
 * BestGuessReleaseWritelockRequest: a request issued by closest edge node to beacon to finish writes with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.04).
 */

#ifndef BESTGUESS_RELEASE_WRITELOCK_REQUEST_H
#define BESTGUESS_RELEASE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessReleaseWritelockRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessReleaseWritelockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        BestGuessReleaseWritelockRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessReleaseWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif