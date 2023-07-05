#include "message/key_validity_directory_message.h"

namespace covered
{
    const std::string KeyValidityDirectoryMessage::kClassName("KeyValidityDirectoryMessage");

    KeyValidityDirectoryMessage::KeyValidityDirectoryMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr) : MessageBase(message_type, source_index, source_addr)
    {
        key_ = key;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
    }

    KeyValidityDirectoryMessage::KeyValidityDirectoryMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValidityDirectoryMessage::~KeyValidityDirectoryMessage() {}

    Key KeyValidityDirectoryMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyValidityDirectoryMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyValidityDirectoryMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    uint32_t KeyValidityDirectoryMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is cooperatively cached + target edge idx
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(uint32_t);
        return msg_payload_size;
    }

    uint32_t KeyValidityDirectoryMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_serialize_size = directory_info_.serialize(msg_payload, size);
        size += directory_info_serialize_size;
        return size - position;
    }

    uint32_t KeyValidityDirectoryMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_deserialize_size = directory_info_.deserialize(msg_payload, size);
        size += directory_info_deserialize_size;
        return size - position;
    }
}