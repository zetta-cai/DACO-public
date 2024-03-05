/*
 * BestGuessRedirectedGetRequest: a request issued by an edge node to the target edge node to get an existing value if any with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.04).
 */

#ifndef BESTGUESS_REDIRECTED_GET_REQUEST_H
#define BESTGUESS_REDIRECTED_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessRedirectedGetRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessRedirectedGetRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessRedirectedGetRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessRedirectedGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif