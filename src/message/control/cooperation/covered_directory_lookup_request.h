/*
 * CoveredDirectoryLookupRequest: a request issued by an edge node to the beacon node to get directory information of a given key with popularity collection and victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef COVERED_DIRECTORY_LOOKUP_REQUEST_H
#define COVERED_DIRECTORY_LOOKUP_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_trackflag_popularity_victimset_message.h"

namespace covered
{
    class CoveredDirectoryLookupRequest : public KeyTrackflagPopularityVictimsetMessage
    {
    public:
        CoveredDirectoryLookupRequest(const Key& key, const bool& is_tracked, const Popularity& local_uncached_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredDirectoryLookupRequest(const DynamicArray& msg_payload);
        virtual ~CoveredDirectoryLookupRequest();
    private:
        static const std::string kClassName;
    };
}

#endif