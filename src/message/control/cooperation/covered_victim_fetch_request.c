#include "message/control/cooperation/covered_victim_fetch_request.h"

namespace covered
{
    const std::string CoveredVictimFetchRequest::kClassName("CoveredVictimFetchRequest");

    CoveredVictimFetchRequest::CoveredVictimFetchRequest(const uint32_t& object_size, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr) : UintVictimsetMessage(object_size, victim_syncset, MessageType::kCoveredVictimFetchRequest, source_index, source_addr, BandwidthUsage(), EventList(), true)
    {
    }

    CoveredVictimFetchRequest::CoveredVictimFetchRequest(const DynamicArray& msg_payload) : UintVictimsetMessage(msg_payload)
    {
    }

    CoveredVictimFetchRequest::~CoveredVictimFetchRequest() {}

    uint32_t CoveredVictimFetchRequest::getObjectSize() const
    {
        checkIsValid_();
        return getUnsignedInteger_();
    }
}