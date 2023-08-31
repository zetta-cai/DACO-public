#include "message/key_popularity_victimset_message.h"

namespace covered
{
    const std::string KeyPopularityVictimsetMessage::kClassName("KeyPopularityVictimsetMessage");

    KeyPopularityVictimsetMessage::KeyPopularityVictimsetMessage(const Key& key, const Popularity& local_uncached_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        key_ = key;
        local_uncached_popularity_ = local_uncached_popularity;
        victim_syncset_ = victim_syncset;
    }

    KeyPopularityVictimsetMessage::KeyPopularityVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyPopularityVictimsetMessage::~KeyPopularityVictimsetMessage() {}

    Key KeyPopularityVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Popularity KeyPopularityVictimsetMessage::getLocalUncachedPopularity() const
    {
        checkIsValid_();
        return local_uncached_popularity_;
    }

    const VictimSyncset& KeyPopularityVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyPopularityVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // TODO: END HERE
        // key payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(Popularity) + ;
        return msg_payload_size;
    }

    uint32_t KeyPopularityVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size - position;
    }

    uint32_t KeyPopularityVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size - position;
    }
}