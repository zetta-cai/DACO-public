/*
 * LocalDelResponse: a request issued by an edge node to a client for LocalDelRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_DEL_RESPONSE_H
#define LOCAL_DEL_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class LocalDelResponse : public MessageBase
    {
    public:
        LocalDelResponse(const Key& key);
        LocalDelResponse(const DynamicArray& msg_payload);
        ~LocalDelResponse();

        Key getKey() const;

        virtual uint32_t getMsgPayloadSize() override;

    private:
        static const std::string kClassName;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
    };
}

#endif