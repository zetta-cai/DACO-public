/*
 * CoveredFinishBlockRequest: a request issued by a beacon node to a closest edge node to finish blocking for writes with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.10.21).
 */

#ifndef COVERED_FINISH_BLOCK_REQUEST_H
#define COVERED_FINISH_BLOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_message.h"

namespace covered
{
    class CoveredFinishBlockRequest : public KeyVictimsetMessage
    {
    public:
        CoveredFinishBlockRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredFinishBlockRequest(const DynamicArray& msg_payload);
        virtual ~CoveredFinishBlockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif