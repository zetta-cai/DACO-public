/*
 * BestGuessRedirectedGetResponse: a response issued by the target edge node to an edge node for RedirectedGetRequest with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.02).
 */

#ifndef BESTGUESS_REDIRECTED_GET_RESPONSE_H
#define BESTGUESS_REDIRECTED_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessRedirectedGetResponse : public KeyValueByteSyncinfoMessage
    {
    public:
        BestGuessRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessRedirectedGetResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessRedirectedGetResponse();

        Hitflag getHitflag() const;
    private:
        static const std::string kClassName;
    };
}

#endif