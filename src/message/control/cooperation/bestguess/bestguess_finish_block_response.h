/*
 * BestGuessFinishBlockResponse: a response issued by a closest edge node to acknowledge FinishBlockRequest from the beacon node with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_FINISH_BLOCK_RESPONSE_H
#define BESTGUESS_FINISH_BLOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessFinishBlockResponse : public KeySyncinfoMessage
    {
    public:
        BestGuessFinishBlockResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        BestGuessFinishBlockResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessFinishBlockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif