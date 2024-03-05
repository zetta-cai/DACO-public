/*
 * BestGuessBestGuessDirectoryLookupRequest: a request issued by an edge node to the beacon node to get directory information of a given key with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_DIRECTORY_LOOKUP_REQUEST_H
#define BESTGUESS_DIRECTORY_LOOKUP_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_syncinfo_message.h"

namespace covered
{
    class BestGuessDirectoryLookupRequest : public KeySyncinfoMessage
    {
    public:
        BestGuessDirectoryLookupRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessDirectoryLookupRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessDirectoryLookupRequest();
    private:
        static const std::string kClassName;
    };
}

#endif