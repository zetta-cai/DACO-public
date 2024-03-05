/*
 * BestGuessPlacementTriggerResponse: a foreground response issued by the beacon node to the closest node to acknowledge BestGuessPlacementTriggerRequest for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef BEST_GUESS_PLACEMENT_TRIGGER_RESPONSE_H
#define BEST_GUESS_PLACEMENT_TRIGGER_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/key_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessPlacementTriggerResponse : public KeyByteSyncinfoMessage
    {
    public:
        BestGuessPlacementTriggerResponse(const Key& key, const bool& is_triggered, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessPlacementTriggerResponse(const DynamicArray& msg_payload);
        virtual ~BestGuessPlacementTriggerResponse();

        bool isTriggered() const;
    private:
        static const std::string kClassName;
    };
}

#endif