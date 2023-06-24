/*
 * DirectoryLookupRequest: a request issued by an edge node to the beacon node to get directory information of a given key.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef DIRECTORY_LOOKUP_REQUEST_H
#define DIRECTORY_LOOKUP_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class DirectoryLookupRequest : public KeyMessage
    {
    public:
        DirectoryLookupRequest(const Key& key, const uint32_t& source_index);
        DirectoryLookupRequest(const DynamicArray& msg_payload);
        virtual ~DirectoryLookupRequest();
    private:
        static const std::string kClassName;
    };
}

#endif