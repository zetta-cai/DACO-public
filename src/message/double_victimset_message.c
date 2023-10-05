#include "message/double_victimset_message.h"

namespace covered
{
    const std::string DoubleVictimsetMessage::kClassName = "DoubleVictimsetMessage";

    DoubleVictimsetMessage::DoubleVictimsetMessage(const VictimSyncset& victim_fetchset, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        victim_fetchset_ = victim_fetchset;
        victim_syncset_ = victim_syncset;
    }

    DoubleVictimsetMessage::DoubleVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    DoubleVictimsetMessage::~DoubleVictimsetMessage()
    {
    }

    const VictimSyncset& DoubleVictimsetMessage::getVictimFetchsetRef() const
    {
        return victim_fetchset_;
    }

    const VictimSyncset& DoubleVictimsetMessage::getVictimSyncsetRef() const
    {
        return victim_syncset_;
    }

    uint32_t DoubleVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // victim fetchset + victim syncset
        return victim_fetchset_.getVictimSyncsetPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize();
    }

    uint32_t DoubleVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t victim_fetctset_serialize_size = victim_fetchset_.serialize(msg_payload, size);
        size += victim_fetctset_serialize_size;
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t DoubleVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t victim_fetchset_deserialize_size = victim_fetchset_.deserialize(msg_payload, size);
        size += victim_fetchset_deserialize_size;
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}