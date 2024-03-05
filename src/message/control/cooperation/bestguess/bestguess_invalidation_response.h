/*
 * BestGuessInvalidationResponse: a response issued by each involved edge node to closest edge node or beacon edge node to acknowledge BestGuessInvalidationRequest.
 * 
 * By Siyuan Sheng (2024.02.04).
 */

#ifndef BESTGUESS_INVALIDATION_RESPONSE_H
#define BESTGUESS_INVALIDATION_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessInvalidationResponse : public KeySyncinfoMessage
    {
    public:
        BestGuessInvalidationResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessInvalidationResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessInvalidationResponse();
    private:
        static const std::string kClassName;
    };
}

#endif