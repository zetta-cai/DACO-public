/*
 * DirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key.
 *
 * NOTE: is_valid_directory_exist indicates the validity of the directory information.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef DIRECTORY_LOOKUP_RESPONSE_H
#define DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "message/key_writeflag_validity_directory_message.h"

namespace covered
{
    class DirectoryLookupResponse : public KeyWriteflagValidityDirectoryMessage
    {
    public:
        DirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency);
        DirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~DirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif