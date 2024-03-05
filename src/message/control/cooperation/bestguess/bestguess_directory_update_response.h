/*
 * BestGuessDirectoryUpdateResponse: a response issued by the beacon node to the closest node to acknowledge BestGuessDirectoryUpdateRequest.
 * 
 * By Siyuan Sheng (2024.02.02).
 */

#ifndef BESTGUESS_DIRECTORY_UPDATE_RESPONSE_H
#define BESTGUESS_DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessDirectoryUpdateResponse : public KeyByteSyncinfoMessage
    {
    public:
        BestGuessDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessDirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessDirectoryUpdateResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif