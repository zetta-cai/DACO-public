/*
 * DoubleVictimsetMessage: the base class for messages with a victim syncset for COVERED's lazy victim fetching and a victim syncset for COVERED's victim synchronization.
 * 
 * By Siyuan Sheng (2023.10.05).
 */

#ifndef DOUBLE_VICTIMSET_MESSAGE_H
#define DOUBLE_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class DoubleVictimsetMessage : public MessageBase
    {
    public:
        DoubleVictimsetMessage(const VictimSyncset& victim_fetchset, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        DoubleVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~DoubleVictimsetMessage();

        const VictimSyncset& getVictimFetchsetRef() const;
        const VictimSyncset& getVictimSyncsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        VictimSyncset victim_fetchset_;
        VictimSyncset victim_syncset_;
    };
}

#endif