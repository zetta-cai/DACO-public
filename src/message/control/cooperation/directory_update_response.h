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
#include "message/key_byte_message.h"

namespace covered
{
    class DirectoryUpdateResponse : public KeyByteMessage
    {
    public:
        DirectoryUpdateResponse(const Key& key, const bool& is_being_written, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        DirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~DirectoryUpdateResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif