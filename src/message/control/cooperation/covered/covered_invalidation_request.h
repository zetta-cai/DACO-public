/*
 * CoveredInvalidationRequest: a request issued by closest edge node or beacon edge node to neighbors to invalidate all cache copies for MSI protocol with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.10.21).
 */

#ifndef COVERED_INVALIDATION_REQUEST_H
#define COVERED_INVALIDATION_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
//#include "message/key_victimset_message.h"
#include "message/key_message.h"

namespace covered
{
    class CoveredInvalidationRequest : public KeyMessage
    {
    public:
        // CoveredInvalidationRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredInvalidationRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredInvalidationRequest(const DynamicArray& msg_payload);
        virtual ~CoveredInvalidationRequest();
    private:
        static const std::string kClassName;
    };
}

#endif