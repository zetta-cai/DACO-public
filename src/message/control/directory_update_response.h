/*
 * DirectoryUpdateResponse: a response issued by the beacon node to the closest node to acknowledge DirectoryUpdateRequest.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef DIRECTORY_UPDATE_RESPONSE_H
#define DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class DirectoryUpdateResponse : public KeyMessage
    {
    public:
        DirectoryUpdateResponse(const Key& key);
        DirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~DirectoryUpdateResponse();
    private:
        static const std::string kClassName;
    };
}

#endif