/*
 * CoveredFghybridHybridFetchedRequest: a foreground request issued by a closest edge node to the beacon edge node to notify the result of excluding-sender hybrid data fetching to trigger COVERED's non-blocking placement notification.
 * 
 * NOTE: foreground request with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.10.07).
 */

#ifndef COVERED_FGHYBRID_HYBRID_FETCHED_REQUEST_H
#define COVERED_FGHYBRID_HYBRID_FETCHED_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_victimset_edgeset_message.h"

namespace covered
{
    class CoveredFghybridHybridFetchedRequest : public KeyValueVictimsetEdgesetMessage
    {
    public:
        CoveredFghybridHybridFetchedRequest(const Key& key, const Value& value, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredFghybridHybridFetchedRequest(const DynamicArray& msg_payload);
        virtual ~CoveredFghybridHybridFetchedRequest();
    private:
        static const std::string kClassName;
    };
}

#endif