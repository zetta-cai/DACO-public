/*
 * CoveredPlacementNotifyRequest: a request issued by a beacon edge node to an involved edge node for COVERED's non-blocking placement notification.
 * 
 * NOTE: CoveredPlacementNotifyRequest does NOT need placement edgeset.
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef COVERED_PLACEMENT_NOTIFY_REQUEST_H
#define COVERED_PLACEMENT_NOTIFY_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_byte_victimset_message.h"

namespace covered
{
    class CoveredPlacementNotifyRequest : public KeyValueByteVictimsetMessage
    {
    public:
        CoveredPlacementNotifyRequest(const Key& key, const Value& value, const bool& is_valid, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency);
        CoveredPlacementNotifyRequest(const DynamicArray& msg_payload);
        virtual ~CoveredPlacementNotifyRequest();

        bool isValid() const;
    private:
        static const std::string kClassName;
    };
}

#endif