#include "message/key_writeflag_validity_directory_victimset_fphint_message.h"

#include <assert.h>

namespace covered
{
    const std::string KeyWriteflagValidityDirectoryVictimsetFphintMessage::kClassName("KeyWriteflagValidityDirectoryVictimsetFphintMessage");

    KeyWriteflagValidityDirectoryVictimsetFphintMessage::KeyWriteflagValidityDirectoryVictimsetFphintMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const FastPathHint& fast_path_hint, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        is_being_written_ = is_being_written;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
        victim_syncset_ = victim_syncset;
        fast_path_hint_ = fast_path_hint;
        assert(fast_path_hint.isValid());
    }

    KeyWriteflagValidityDirectoryVictimsetFphintMessage::KeyWriteflagValidityDirectoryVictimsetFphintMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyWriteflagValidityDirectoryVictimsetFphintMessage::~KeyWriteflagValidityDirectoryVictimsetFphintMessage() {}

    Key KeyWriteflagValidityDirectoryVictimsetFphintMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyWriteflagValidityDirectoryVictimsetFphintMessage::isBeingWritten() const
    {
        checkIsValid_();
        return is_being_written_;
    }

    bool KeyWriteflagValidityDirectoryVictimsetFphintMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyWriteflagValidityDirectoryVictimsetFphintMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    const VictimSyncset& KeyWriteflagValidityDirectoryVictimsetFphintMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    const FastPathHint& KeyWriteflagValidityDirectoryVictimsetFphintMessage::getFastPathHintRef() const
    {
        checkIsValid_();
        assert(fast_path_hint_.isValid());
        return fast_path_hint_;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetFphintMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is being written + is cooperatively cached + directory info (target edge idx) + victim syncset + fast-path hint
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize() + fast_path_hint_.getFastPathHintPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetFphintMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
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
        uint32_t fast_path_hint_serialize_size = fast_path_hint_.serialize(msg_payload, size);
        size += fast_path_hint_serialize_size;
        return size - position;
    }

    uint32_t KeyWriteflagValidityDirectoryVictimsetFphintMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
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
        uint32_t fast_path_hint_deserialize_size = fast_path_hint_.deserialize(msg_payload, size);
        size += fast_path_hint_deserialize_size;
        return size - position;
    }
}