/*
 * BestGuessBgplaceDirectoryUpdateResponse: a background response issued by the beacon node to the closest node to acknowledge BestGuessBgplaceDirectoryUpdateRequest.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BGPLACE_BESTGUESS_DIRECTORY_UPDATE_RESPONSE_H
#define BGPLACE_BESTGUESS_DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessBgplaceDirectoryUpdateResponse : public KeyByteSyncinfoMessage
    {
    public:
        BestGuessBgplaceDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        BestGuessBgplaceDirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessBgplaceDirectoryUpdateResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif