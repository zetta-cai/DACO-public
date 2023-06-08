/*
 * DirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key.
 *
 * NOTE: is_directory_exist indicates the validity of the directory information.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef DIRECTORY_LOOKUP_RESPONSE_H
#define DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory_info.h"
#include "message/key_existence_directory_message.h"

namespace covered
{
    class DirectoryLookupResponse : public KeyExistenceDirectoryMessage
    {
    public:
        DirectoryLookupResponse(const Key& key, const bool& is_directory_exist, const DirectoryInfo& directory_info);
        DirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~DirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif