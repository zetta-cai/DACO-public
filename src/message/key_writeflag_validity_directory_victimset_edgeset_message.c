#include "message/key_writeflag_validity_directory_victimset_edgeset_message.h"

namespace covered
{
    const std::string KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::kClassName("KeyWriteflagValidityDirectoryVictimsetEdgesetMessage");

    KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        is_being_written_ = is_being_written;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
        victim_syncset_ = victim_syncset;
        edgeset_ = edgeset;
    }

    KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::~KeyWriteflagValidityDirectoryVictimsetEdgesetMessage() {}

    Key KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::isBeingWritten() const
    {
        checkIsValid_();
        return is_being_written_;
    }

    bool KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    const VictimSyncset& KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    const Edgeset& KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getVictimSyncsetBytes() const
    {
        checkIsValid_();
        return victim_syncset_.getVictimSyncsetPayloadSize();
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is being written + is cooperatively cached + directory info (target edge idx) + victim syncset + edgeset syncset
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
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
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
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
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}