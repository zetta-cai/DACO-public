/*
 * BestGuessBgplacePlacementNotifyRequest: a background request issued by a beacon edge node to the best-guess placement edge node with vtime synchronization for BestGuess.
 * 
 * By Siyuan Sheng (2024.02.03).
 */

#ifndef BESTGUESS_BGPLACE_PLACEMENT_NOTIFY_REQUEST_H
#define BESTGUESS_BGPLACE_PLACEMENT_NOTIFY_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/key_value_byte_syncinfo_message.h"

namespace covered
{
    class BestGuessBgplacePlacementNotifyRequest : public KeyValueByteSyncinfoMessage
    {
    public:
        BestGuessBgplacePlacementNotifyRequest(const Key& key, const Value& value, const bool& is_valid, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        BestGuessBgplacePlacementNotifyRequest(const DynamicArray& msg_payload);
        virtual ~BestGuessBgplacePlacementNotifyRequest();

        bool isValid() const;
    private:
        static const std::string kClassName;
    };
}

#endif