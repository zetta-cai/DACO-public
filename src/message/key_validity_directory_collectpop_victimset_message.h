/*
 * KeyValidityDirectoryCollectpopVictimsetMessage: the base class for messages each with a key, a validity boolean, a directory information, collected popularity, and victim syncset.
 *
 * NOTE: the validity boolean indicates whether the directory information is valid.
 * 
 * NOTE: the collected popularity will be serialized/deserialized ONLY if validity = false (i.e., is_admit = false), where collected_popularity_.isTracked() indicates that whether the evicted key is selected for metadata preservation.
 * 
 * By Siyuan Sheng (2023.09.07).
 */

#ifndef KEY_VALIDITY_DIRECTORY_COLLECTPOP_VICTIMSET_MESSAGE_H
#define KEY_VALIDITY_DIRECTORY_COLLECTPOP_VICTIMSET_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValidityDirectoryCollectpopVictimsetMessage : public MessageBase
    {
    public:
        KeyValidityDirectoryCollectpopVictimsetMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValidityDirectoryCollectpopVictimsetMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValidityDirectoryCollectpopVictimsetMessage(const DynamicArray& msg_payload);
        virtual ~KeyValidityDirectoryCollectpopVictimsetMessage();

        Key getKey() const;
        bool isValidDirectoryExist() const;
        DirectoryInfo getDirectoryInfo() const;
        const CollectedPopularity& getCollectedPopularityRef() const; // ONLY if is_valid_directory_exist_ = false
        const VictimSyncset& getVictimSyncsetRef() const;

        // Used by BandwidthUsage
        virtual uint32_t getVictimSyncsetBytes() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        
        // Directory information
        bool is_valid_directory_exist_;
        DirectoryInfo directory_info_;

        CollectedPopularity collected_popularity_; // For popularity collection with flag indicating whether key is tracked by the sender (i.e., whether local_uncached_popularity_ is valid) (ONLY if is_valid_directory_exist_ = false)
        VictimSyncset victim_syncset_; // For victim synchronization
    };
}

#endif