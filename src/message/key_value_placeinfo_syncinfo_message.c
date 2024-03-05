#include "message/key_value_placeinfo_syncinfo_message.h"

namespace covered
{
    const std::string KeyValuePlaceinfoSyncinfoMessage::kClassName("KeyValuePlaceinfoSyncinfoMessage");

    KeyValuePlaceinfoSyncinfoMessage::KeyValuePlaceinfoSyncinfoMessage(const Key& key, const Value& value, const BestGuessPlaceinfo& placeinfo, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        value_ = value;
        placeinfo_ = placeinfo;
        syncinfo_ = syncinfo;
    }

    KeyValuePlaceinfoSyncinfoMessage::KeyValuePlaceinfoSyncinfoMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValuePlaceinfoSyncinfoMessage::~KeyValuePlaceinfoSyncinfoMessage() {}

    Key KeyValuePlaceinfoSyncinfoMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValuePlaceinfoSyncinfoMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    BestGuessPlaceinfo KeyValuePlaceinfoSyncinfoMessage::getPlaceinfo() const
    {
        checkIsValid_();
        return placeinfo_;
    }

    BestGuessSyncinfo KeyValuePlaceinfoSyncinfoMessage::getSyncinfo() const
    {
        checkIsValid_();
        return syncinfo_;
    }

    uint32_t KeyValuePlaceinfoSyncinfoMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + placeinfo + syncinfo
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + placeinfo_.getPlaceinfoPayloadSize() + syncinfo_.getSyncinfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValuePlaceinfoSyncinfoMessage::getMsgBandwidthSizeInternal_() const
    {
        // key payload + ideal value content size + placeinfo + syncinfo
        uint32_t msg_bandwidth_size = key_.getKeyPayloadSize() + value_.getValuesize() + placeinfo_.getPlaceinfoPayloadSize() + syncinfo_.getSyncinfoPayloadSize();
        return msg_bandwidth_size;
    }

    uint32_t KeyValuePlaceinfoSyncinfoMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint32_t placeinfo_serialize_size = placeinfo_.serialize(msg_payload, size);
        size += placeinfo_serialize_size;
        uint32_t syncinfo_serialize_size = syncinfo_.serialize(msg_payload, size);
        size += syncinfo_serialize_size;
        return size - position;
    }

    uint32_t KeyValuePlaceinfoSyncinfoMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint32_t placeinfo_deserialize_size = placeinfo_.deserialize(msg_payload, size);
        size += placeinfo_deserialize_size;
        uint32_t syncinfo_deserialize_size = syncinfo_.deserialize(msg_payload, size);
        size += syncinfo_deserialize_size;
        return size - position;
    }
}