/*
 * DirectoryUpdateRequest: a request issued by the closest node to the beacon node to update directory information of a given key.
 *
 * NOTE: is_valid_directory_exist indicates whether to add or delete the directory information.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef DIRECTORY_UPDATE_REQUEST_H
#define DIRECTORY_UPDATE_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_existence_directory_message.h"

namespace covered
{
    class DirectoryUpdateRequest : public KeyExistenceDirectoryMessage
    {
    public:
        DirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info);
        DirectoryUpdateRequest(const DynamicArray& msg_payload);
        virtual ~DirectoryUpdateRequest();
    private:
        static const std::string kClassName;
    };
}

#endif