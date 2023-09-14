/*
 * CoveredCoveredDirectoryUpdateResponse: a response issued by the beacon node to the closest node to acknowledge CoveredDirectoryUpdateRequest with victim synchronization for COVERED.
 * 
 * By Siyuan Sheng (2023.09.08).
 */

#ifndef COVERED_DIRECTORY_UPDATE_RESPONSE_H
#define COVERED_DIRECTORY_UPDATE_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_byte_victimset_message.h"

namespace covered
{
    class CoveredDirectoryUpdateResponse : public KeyByteVictimsetMessage
    {
    public:
        CoveredDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        CoveredDirectoryUpdateResponse(const DynamicArray& msg_payload);
        virtual ~CoveredDirectoryUpdateResponse();

        bool isBeingWritten() const;
    private:
        static const std::string kClassName;
    };
}

#endif