/*
 * BestGuessDirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key with vtime synchronization for BestGuess.
 *
 * NOTE: is_valid_directory_exist indicates the validity of the directory information.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_DIRECTORY_LOOKUP_RESPONSE_H
#define BESTGUESS_DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "message/key_writeflag_validity_directory_syncinfo_message.h"

namespace covered
{
    class BestGuessDirectoryLookupResponse : public KeyWriteflagValidityDirectorySyncinfoMessage
    {
    public:
        BestGuessDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessDirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessDirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif