#include "message/key_writeflag_validity_directory_message.h"

namespace covered
{
    const std::string KeyWriteflagValidityDirectoryMessage::kClassName("KeyWriteflagValidityDirectoryMessage");

    KeyWriteflagValidityDirectoryMessage::KeyWriteflagValidityDirectoryMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        is_being_written_ = is_being_written;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
    }

    KeyWriteflagValidityDirectoryMessage::KeyWriteflagValidityDirectoryMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyWriteflagValidityDirectoryMessage::~KeyWriteflagValidityDirectoryMessage() {}

    Key KeyWriteflagValidityDirectoryMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyWriteflagValidityDirectoryMessage::isBeingWritten() const
    {
        checkIsValid_();
        return is_being_written_;
    }

    bool KeyWriteflagValidityDirectoryMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyWriteflagValidityDirectoryMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    uint32_t KeyWriteflagValidityDirectoryMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is being written + is cooperatively cached + directory info (target edge idx)
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyWriteflagValidityDirectoryMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&is_being_written_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.deserialize(size, (const char*)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_serialize_size = directory_info_.serialize(msg_payload, size);
        size += directory_info_serialize_size;
        return size - position;
    }

    uint32_t KeyWriteflagValidityDirectoryMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&is_being_written_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.serialize(size, (char *)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_deserialize_size = directory_info_.deserialize(msg_payload, size);
        size += directory_info_deserialize_size;
        return size - position;
    }
}