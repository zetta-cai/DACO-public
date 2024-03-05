/*
 * CoveredAcquireWritelockRequest: a request issued by closest edge node to the beacon node to acquire permission for a write with selective popularity aggregation and victim synchronizatino for COVERED.
 * 
 * By Siyuan Sheng (2023.09.09).
 */

#ifndef COVERED_ACQUIRE_WRITELOCK_REQUEST_H
#define COVERED_ACQUIRE_WRITELOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_collectpop_victimset_message.h"

namespace covered
{
    class CoveredAcquireWritelockRequest : public KeyCollectpopVictimsetMessage
    {
    public:
        CoveredAcquireWritelockRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredAcquireWritelockRequest(const DynamicArray& msg_payload);
        virtual ~CoveredAcquireWritelockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif