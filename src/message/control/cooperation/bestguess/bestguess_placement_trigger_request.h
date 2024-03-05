/*
 * BestGuessPlacementTriggerRequest: a foreground request issued by the closest node to the beacon node to try to trigger best-guess placement for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef BEST_GUESS_PLACEMENT_TRIGGER_REQUEST_H
#define BEST_GUESS_PLACEMENT_TRIGGER_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/key_value_placeinfo_syncinfo_message.h"

namespace covered
{
    class BestGuessPlacementTriggerRequest : public KeyValuePlaceinfoSyncinfoMessage
    {
    public:
        BestGuessPlacementTriggerRequest(const Key& key, const Value& value, const BestGuessPlaceinfo& placeinfo, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessPlacementTriggerRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessPlacementTriggerRequest();
    private:
        static const std::string kClassName;
    };
}

#endif