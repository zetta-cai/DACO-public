/*
 * CoveredReleaseWritelockRequest: a request issued by closest edge node to beacon to finish writes with selective popularity aggregation and victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.09).
 */

#ifndef COVERED_RELEASE_WRITELOCK_REQUEST_H
#define COVERED_RELEASE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_collectpop_victimset_message.h"

namespace covered
{
    class CoveredReleaseWritelockRequest : public KeyCollectpopVictimsetMessage
    {
    public:
        CoveredReleaseWritelockRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredReleaseWritelockRequest(const DynamicArray& msg_payload);
        virtual ~CoveredReleaseWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif