/*
 * KeyExistenceDirectoryMessage: the base class for messages each with a key, an existence boolean, and a directory information.
 *
 * NOTE: the existence boolean indicates whether the key is cooperatively cached.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef KEY_EXISTENCE_DIRECTORY_MESSAGE_H
#define KEY_EXISTENCE_DIRECTORY_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "cooperation/directory_info.h"
#include "message/message_base.h"

namespace covered
{
    class KeyExistenceDirectoryMessage : public MessageBase
    {
    public:
        KeyExistenceDirectoryMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const MessageType& message_type);
        KeyExistenceDirectoryMessage(const DynamicArray& msg_payload);
        virtual ~KeyExistenceDirectoryMessage();

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