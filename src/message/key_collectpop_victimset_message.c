#include "message/key_collectpop_victimset_message.h"

namespace covered
{
    const std::string KeyCollectpopVictimsetMessage::kClassName("KeyCollectpopVictimsetMessage");

    KeyCollectpopVictimsetMessage::KeyCollectpopVictimsetMessage(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        collected_popularity_ = collected_popularity;
        victim_syncset_ = victim_syncset;
    }

    KeyCollectpopVictimsetMessage::KeyCollectpopVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyCollectpopVictimsetMessage::~KeyCollectpopVictimsetMessage() {}

    Key KeyCollectpopVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    const CollectedPopularity& KeyCollectpopVictimsetMessage::getCollectedPopularityRef() const
    {
        checkIsValid_();
        return collected_popularity_;
    }

    const VictimSyncset& KeyCollectpopVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyCollectpopVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + collected_popularity + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + collected_popularity_.getCollectedPopularityPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyCollectpopVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t collected_popularity_serialize_size = collected_popularity_.serialize(msg_payload, size);
        size += collected_popularity_serialize_size;
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyCollectpopVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t collected_popularity_deserialize_size = collected_popularity_.deserialize(msg_payload, size);
        size += collected_popularity_deserialize_size;
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}