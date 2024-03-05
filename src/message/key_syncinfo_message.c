#include "message/key_syncinfo_message.h"

namespace covered
{
    const std::string KeySyncinfoMessage::kClassName("KeySyncinfoMessage");

    KeySyncinfoMessage::KeySyncinfoMessage(const Key& key, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        syncinfo_ = syncinfo;
    }

    KeySyncinfoMessage::KeySyncinfoMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeySyncinfoMessage::~KeySyncinfoMessage() {}

    Key KeySyncinfoMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    BestGuessSyncinfo KeySyncinfoMessage::getSyncinfo() const
    {
        checkIsValid_();
        return syncinfo_;
    }

    uint32_t KeySyncinfoMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + syncinfo
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + syncinfo_.getSyncinfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeySyncinfoMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t syncinfo_serialize_size = syncinfo_.serialize(msg_payload, size);
        size += syncinfo_serialize_size;
        return size - position;
    }

    uint32_t KeySyncinfoMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t syncinfo_deserialize_size = syncinfo_.deserialize(msg_payload, size);
        size += syncinfo_deserialize_size;
        return size - position;
    }
}