/*
 * KeyWriteflagValidityDirectoryVictimsetFphintMessage: the base class for messages each with a key, a write flag, a validity boolean, a directory information, victim syncset, and fast-path hint.
 *
 * NOTE: the write flag indicates whether key is being written.
 * 
 * By Siyuan Sheng (2023.11.11).
 */

#ifndef KEY_WRITEFLAG_VALIDITY_DIRECTORY_VICTIMSET_FPIHINT_MESSAGE_H
#define KEY_WRITEFLAG_VALIDITY_DIRECTORY_VICTIMSET_FPIHINT_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "core/popularity/fast_path_hint.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyWriteflagValidityDirectoryVictimsetFphintMessage : public MessageBase
    {
    public:
        KeyWriteflagValidityDirectoryVictimsetFphintMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const FastPathHint& fast_path_hint, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyWriteflagValidityDirectoryVictimsetFphintMessage(const DynamicArray& msg_payload);
        virtual ~KeyWriteflagValidityDirectoryVictimsetFphintMessage();

        Key getKey() const;
        bool isBeingWritten() const;
        bool isValidDirectoryExist() const;
        DirectoryInfo getDirectoryInfo() const;
        const VictimSyncset& getVictimSyncsetRef() const;
        const FastPathHint& getFastPathHintRef() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        
        // Directory information
        bool is_being_written_;
        bool is_valid_directory_exist_;
        DirectoryInfo directory_info_;

        VictimSyncset victim_syncset_; // For victim synchronization

        FastPathHint fast_path_hint_; // For fast-path single-placement calculation
    };
}

#endif