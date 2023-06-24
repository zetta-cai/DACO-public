/*
 * KeyWriteflagValidityDirectoryMessage: the base class for messages each with a key, a write flag, a validity boolean, and a directory information.
 *
 * NOTE: the write flag indicates whether key is being written.
 * 
 * By Siyuan Sheng (2023.06.14).
 */

#ifndef KEY_WRITEFLAG_VALIDITY_DIRECTORY_MESSAGE_H
#define KEY_WRITEFLAG_VALIDITY_DIRECTORY_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory_info.h"
#include "message/message_base.h"

namespace covered
{
    class KeyWriteflagValidityDirectoryMessage : public MessageBase
    {
    public:
        KeyWriteflagValidityDirectoryMessage(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const MessageType& message_type, const uint32_t& source_index);
        KeyWriteflagValidityDirectoryMessage(const DynamicArray& msg_payload);
        virtual ~KeyWriteflagValidityDirectoryMessage();

        Key getKey() const;
        bool isBeingWritten() const;
        bool isValidDirectoryExist() const;
        DirectoryInfo getDirectoryInfo() const;
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
    };
}

#endif