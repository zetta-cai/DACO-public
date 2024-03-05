/*
 * CoveredBgplacePlacementNotifyRequest: a background request issued by a beacon edge node to an involved edge node for COVERED's non-blocking placement notification.
 * 
 * NOTE: background request without placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef COVERED_BGPLACE_PLACEMENT_NOTIFY_REQUEST_H
#define COVERED_BGPLACE_PLACEMENT_NOTIFY_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_byte_victimset_message.h"

namespace covered
{
    class CoveredBgplacePlacementNotifyRequest : public KeyValueByteVictimsetMessage
    {
    public:
        CoveredBgplacePlacementNotifyRequest(const Key& key, const Value& value, const bool& is_valid, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredBgplacePlacementNotifyRequest(const DynamicArray& msg_payload);
        virtual ~CoveredBgplacePlacementNotifyRequest();

        bool isValid() const;
    private:
        static const std::string kClassName;
    };
}

#endif