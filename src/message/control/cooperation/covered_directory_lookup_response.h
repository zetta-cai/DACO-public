/*
 * CoveredDirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef COVERED_DIRECTORY_LOOKUP_RESPONSE_H
#define COVERED_DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_victimset_writeflag_validity_directory_message.h"

namespace covered
{
    class CoveredDirectoryLookupResponse : public KeyVictimsetWriteflagValidityDirectoryMessage
    {
    public:
        CoveredDirectoryLookupResponse(const Key& key, const VictimSyncset& victim_syncset, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredDirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~CoveredDirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif