/*
 * KeyWriteflagValidityDirectoryVictimsetEdgesetMessage: the base class for messages each with a key, a write flag, a validity boolean, a directory information, victim syncset, and an edgeset.
 *
 * NOTE: the write flag indicates whether key is being written.
 * 
 * By Siyuan Sheng (2023.10.07).
 */

#ifndef KEY_WRITEFLAG_VALIDITY_DIRECTORY_VICTIMSET_EDGESET_MESSAGE_H
#define KEY_WRITEFLAG_VALIDITY_DIRECTORY_VICTIMSET_EDGESET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "core/popularity/edgeset.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyWriteflagValidityDirectoryVictimsetEdgesetMessage : public MessageBase
    {
    public:
        KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(const DynamicArray& msg_payload);
        virtual ~KeyWriteflagValidityDirectoryVictimsetEdgesetMessage();

        Key getKey() const;
        bool isBeingWritten() const;
        bool isValidDirectoryExist() const;
        DirectoryInfo getDirectoryInfo() const;
        const VictimSyncset& getVictimSyncsetRef() const;
        const Edgeset& getEdgesetRef() const;

        // Used by BandwidthUsage
        virtual uint32_t getVictimSyncsetBytes() const;
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
        Edgeset edgeset_; // For non-blocking placement deployment
    };
}

#endif