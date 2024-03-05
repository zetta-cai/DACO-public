#include "message/control/cooperation/bestguess/bestguess_bgplace_placement_notify_request.h"

#include <assert.h>

namespace covered
{
    const std::string BestGuessBgplacePlacementNotifyRequest::kClassName("BestGuessBgplacePlacementNotifyRequest");

    BestGuessBgplacePlacementNotifyRequest::BestGuessBgplacePlacementNotifyRequest(const Key& key, const Value& value, const bool& is_valid, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValueByteSyncinfoMessage(key, value, static_cast<uint8_t>(is_valid), syncinfo, MessageType::kBestGuessBgplacePlacementNotifyRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessBgplacePlacementNotifyRequest::BestGuessBgplacePlacementNotifyRequest(const DynamicArray& msg_payload) : KeyValueByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessBgplacePlacementNotifyRequest::~BestGuessBgplacePlacementNotifyRequest() {}

    bool BestGuessBgplacePlacementNotifyRequest::isValid() const
    {
        return static_cast<bool>(getByte_());
    }
}