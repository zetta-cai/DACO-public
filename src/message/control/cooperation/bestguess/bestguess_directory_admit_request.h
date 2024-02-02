/*
 * BestGuessDirectoryAdmitRequest: a request issued by the closest node to the beacon node to admit directory information of a given key with vtime synchronization for BestGuess after triggering best-guess placement.
 *
 * NOTE: is_valid_directory_exist indicates whether to add or delete the directory information.
 * 
 * By Siyuan Sheng (2024.02.02).
 */

#ifndef BESTGUESS_DIRECTORY_ADMIT_REQUEST_H
#define BESTGUESS_DIRECTORY_ADMIT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_validity_directory_syncinfo_message.h"

namespace covered
{
    class BestGuessDirectoryAdmitRequest : public KeyValidityDirectorySyncinfoMessage
    {
    public:
        BestGuessDirectoryAdmitRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& bestguess_syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        BestGuessDirectoryAdmitRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessDirectoryAdmitRequest();
    private:
        static const std::string kClassName;
    };
}

#endif