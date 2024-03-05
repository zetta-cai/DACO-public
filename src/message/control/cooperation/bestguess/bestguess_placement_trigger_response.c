#include "message/control/cooperation/bestguess/bestguess_placement_trigger_response.h"

namespace covered
{
    const std::string BestGuessPlacementTriggerResponse::kClassName("BestGuessPlacementTriggerResponse");

    BestGuessPlacementTriggerResponse::BestGuessPlacementTriggerResponse(const Key& key, const bool& is_triggered, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyByteSyncinfoMessage(key, static_cast<uint8_t>(is_triggered), syncinfo, MessageType::kBestGuessPlacementTriggerResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessPlacementTriggerResponse::BestGuessPlacementTriggerResponse(const DynamicArray& msg_payload) : KeyByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessPlacementTriggerResponse::~BestGuessPlacementTriggerResponse() {}

    bool BestGuessPlacementTriggerResponse::isTriggered() const
    {
        return static_cast<bool>(getByte_());
    }
}