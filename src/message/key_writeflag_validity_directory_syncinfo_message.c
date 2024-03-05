#include "message/key_writeflag_validity_directory_syncinfo_message.h"

namespace covered
{
    const std::string KeyWriteflagValidityDirectorySyncinfoMessage::kClassName("KeyWriteflagValidityDirectorySyncinfoMessage");

    KeyWriteflagValidityDirectorySyncinfoMessage::KeyWriteflagValidityDirectorySyncinfoMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& syncinfo, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        is_being_written_ = is_being_written;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
        syncinfo_ = syncinfo;
    }

    KeyWriteflagValidityDirectorySyncinfoMessage::KeyWriteflagValidityDirectorySyncinfoMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyWriteflagValidityDirectorySyncinfoMessage::~KeyWriteflagValidityDirectorySyncinfoMessage() {}

    Key KeyWriteflagValidityDirectorySyncinfoMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyWriteflagValidityDirectorySyncinfoMessage::isBeingWritten() const
    {
        checkIsValid_();
        return is_being_written_;
    }

    bool KeyWriteflagValidityDirectorySyncinfoMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyWriteflagValidityDirectorySyncinfoMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    BestGuessSyncinfo KeyWriteflagValidityDirectorySyncinfoMessage::getSyncinfo() const
    {
        checkIsValid_();
        return syncinfo_;
    }

    uint32_t KeyWriteflagValidityDirectorySyncinfoMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is being written + is cooperatively cached + directory info (target edge idx) + syncinfo
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize() + syncinfo_.getSyncinfoPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyWriteflagValidityDirectorySyncinfoMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
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
        uint32_t syncinfo_serialize_size = syncinfo_.serialize(msg_payload, size);
        size += syncinfo_serialize_size;
        return size - position;
    }

    uint32_t KeyWriteflagValidityDirectorySyncinfoMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
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
        uint32_t syncinfo_deserialize_size = syncinfo_.deserialize(msg_payload, size);
        size += syncinfo_deserialize_size;
        return size - position;
    }
}