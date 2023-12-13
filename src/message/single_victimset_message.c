#include "message/single_victimset_message.h"

namespace covered
{
    const std::string SingleVictimsetMessage::kClassName = "SingleVictimsetMessage";

    SingleVictimsetMessage::SingleVictimsetMessage(const VictimSyncset& victim_fetchset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        victim_fetchset_ = victim_fetchset;
    }

    SingleVictimsetMessage::SingleVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    SingleVictimsetMessage::~SingleVictimsetMessage()
    {
    }

    const VictimSyncset& SingleVictimsetMessage::getVictimFetchsetRef() const
    {
        return victim_fetchset_;
    }

    uint32_t SingleVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // victim fetchset
        return victim_fetchset_.getVictimSyncsetPayloadSize();
    }

    uint32_t SingleVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t victim_fetctset_serialize_size = victim_fetchset_.serialize(msg_payload, size);
        size += victim_fetctset_serialize_size;
        return size - position;
    }

    uint32_t SingleVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t victim_fetchset_deserialize_size = victim_fetchset_.deserialize(msg_payload, size);
        size += victim_fetchset_deserialize_size;
        return size - position;
    }
}