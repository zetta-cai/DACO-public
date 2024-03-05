/*
 * KeyValidityDirectoryMessage: the base class for messages each with a key, a validity boolean, and a directory information.
 *
 * NOTE: the validity boolean indicates whether the directory information is valid.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef KEY_VALIDITY_DIRECTORY_MESSAGE_H
#define KEY_VALIDITY_DIRECTORY_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "message/message_base.h"

namespace covered
{
    class KeyValidityDirectoryMessage : public MessageBase
    {
    public:
        KeyValidityDirectoryMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        KeyValidityDirectoryMessage(const DynamicArray& msg_payload);
        virtual ~KeyValidityDirectoryMessage();

        Key getKey() const;
        bool isValidDirectoryExist() const;
        DirectoryInfo getDirectoryInfo() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        
        // Directory information
        bool is_valid_directory_exist_;
        DirectoryInfo directory_info_;
    };
}

#endif