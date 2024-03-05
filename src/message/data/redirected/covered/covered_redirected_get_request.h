/*
 * CoveredRedirectedGetRequest: a request issued by an edge node to the target edge node to get an existing value if any with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.13).
 */

#ifndef COVERED_REDIRECTED_GET_REQUEST_H
#define COVERED_REDIRECTED_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredRedirectedGetRequest : public KeyVictimsetMessage
    {
    public:
        CoveredRedirectedGetRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredRedirectedGetRequest(const DynamicArray& msg_payload);
        virtual ~CoveredRedirectedGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif