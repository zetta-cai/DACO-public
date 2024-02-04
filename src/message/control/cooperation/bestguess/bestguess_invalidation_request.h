/*
 * BestGuessInvalidationRequest: a request issued by closest edge node or beacon edge node to neighbors to invalidate all cache copies for MSI protocol with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.04).
 */

#ifndef BESTGUESS_INVALIDATION_REQUEST_H
#define BESTGUESS_INVALIDATION_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessInvalidationRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessInvalidationRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        BestGuessInvalidationRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessInvalidationRequest();
    private:
        static const std::string kClassName;
    };
}

#endif