/*
 * CoveredDirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.06).
 */

#ifndef COVERED_DIRECTORY_LOOKUP_RESPONSE_H
#define COVERED_DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_writeflag_validity_directory_victimset_message.h"

namespace covered
{
    class CoveredDirectoryLookupResponse : public KeyWriteflagValidityDirectoryVictimsetMessage
    {
    public:
        CoveredDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredDirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~CoveredDirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif