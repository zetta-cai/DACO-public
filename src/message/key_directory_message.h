/*
 * KeyDirectoryMessage: the base class for messages each with a key and a directory information (i.e., the edge index of neighbor edge ndoe).
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef KEY_DIRECTORY_MESSAGE_H
#define KEY_DIRECTORY_MESSAGE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/message_base.h"

namespace covered
{
    class KeyDirectoryMessage : public MessageBase
    {
    public:
        KeyDirectoryMessage(const Key& key, const uint32_t& neighbor_edge_idx, const MessageType& message_type);
        KeyDirectoryMessage(const DynamicArray& msg_payload);
        virtual ~KeyDirectoryMessage();

        Key getKey() const;
        uint32_t getNeighborEdgeIdx() const;
    private:
        static const std::string kClassName;

        virtual uint32_t getMsgPayloadSizeInternal_() const override;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        uint32_t neighbor_edge_idx_;
    };
}

#endif