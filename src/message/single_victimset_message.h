/*
 * SingleVictimsetMessage: the base class for messages with a victim syncset for COVERED's lazy victim fetching.
 * 
 * By Siyuan Sheng (2023.12.13).
 */

#ifndef SINGLE_VICTIMSET_MESSAGE_H
#define SINGLE_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class SingleVictimsetMessage : public MessageBase
    {
    public:
        SingleVictimsetMessage(const VictimSyncset& victim_fetchset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        SingleVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~SingleVictimsetMessage();

        const VictimSyncset& getVictimFetchsetRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        VictimSyncset victim_fetchset_;
    };
}

#endif