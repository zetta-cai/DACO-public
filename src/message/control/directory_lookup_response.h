/*
 * DirectoryLookupResponse: a response issued by the beacon node to an edge node to reply directory information of a given key.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef DIRECTORY_LOOKUP_RESPONSE_H
#define DIRECTORY_LOOKUP_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_directory_message.h"

namespace covered
{
    class DirectoryLookupResponse : public KeyDirectoryMessage
    {
    public:
        DirectoryLookupResponse(const Key& key, const bool& is_directory_exist, const uint32_t& neighbor_edge_idx);
        DirectoryLookupResponse(const DynamicArray& msg_payload);
        virtual ~DirectoryLookupResponse();
    private:
        static const std::string kClassName;
    };
}

#endif