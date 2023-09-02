#include "message/key_trackflag_popularity_victimset_message.h"

namespace covered
{
    const std::string KeyTrackflagPopularityVictimsetMessage::kClassName("KeyTrackflagPopularityVictimsetMessage");

    KeyTrackflagPopularityVictimsetMessage::KeyTrackflagPopularityVictimsetMessage(const Key& key, const bool& is_tracked, const Popularity& local_uncached_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        key_ = key;
        is_tracked_ = is_tracked;
        local_uncached_popularity_ = local_uncached_popularity;
        victim_syncset_ = victim_syncset;
    }

    KeyTrackflagPopularityVictimsetMessage::KeyTrackflagPopularityVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyTrackflagPopularityVictimsetMessage::~KeyTrackflagPopularityVictimsetMessage() {}

    Key KeyTrackflagPopularityVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyTrackflagPopularityVictimsetMessage::isTracked() const
    {
        checkIsValid_();
        return is_tracked_;
    }

    Popularity KeyTrackflagPopularityVictimsetMessage::getLocalUncachedPopularity() const
    {
        checkIsValid_();
        return local_uncached_popularity_;
    }

    const VictimSyncset& KeyTrackflagPopularityVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyTrackflagPopularityVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + track flag + local uncached popularity + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(Popularity) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyTrackflagPopularityVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&is_tracked_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.deserialize(size, (const char*)&local_uncached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyTrackflagPopularityVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char*)&is_tracked_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.serialize(size, (char*)&local_uncached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}