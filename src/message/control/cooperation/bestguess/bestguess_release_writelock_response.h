/*
 * BestGuessReleaseWritelockResponse: a response issued by beacon edge node to acknowledge BestGuessReleaseWritelockRequest with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.04).
 */

#ifndef BESTGUESS_RELEASE_WRITELOCK_RESPONSE_H
#define BESTGUESS_RELEASE_WRITELOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessReleaseWritelockResponse : public KeySyncinfoMessage
    {
    public:
        BestGuessReleaseWritelockResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessReleaseWritelockResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessReleaseWritelockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif