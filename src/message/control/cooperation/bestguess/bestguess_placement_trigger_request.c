#include "message/control/cooperation/bestguess/bestguess_placement_trigger_request.h"

namespace covered
{
    const std::string BestGuessPlacementTriggerRequest::kClassName("BestGuessPlacementTriggerRequest");

    BestGuessPlacementTriggerRequest::BestGuessPlacementTriggerRequest(const Key& key, const Value& value, const BestGuessPlaceinfo& placeinfo, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValuePlaceinfoSyncinfoMessage(key, value, placeinfo, syncinfo, MessageType::kBestGuessPlacementTriggerRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessPlacementTriggerRequest::BestGuessPlacementTriggerRequest(const DynamicArray& msg_payload) : KeyValuePlaceinfoSyncinfoMessage(msg_payload)
    {
    }

    BestGuessPlacementTriggerRequest::~BestGuessPlacementTriggerRequest() {}
}