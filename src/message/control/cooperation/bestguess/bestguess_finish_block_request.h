/*
 * BestGuessFinishBlockRequest: a request issued by a beacon node to a closest edge node to finish blocking for writes with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_FINISH_BLOCK_REQUEST_H
#define BESTGUESS_FINISH_BLOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessFinishBlockRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessFinishBlockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessFinishBlockRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessFinishBlockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif