/*
 * LocalGetResponse: a response issued by an edge node to a client for LocalGetRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_GET_RESPONSE_H
#define LOCAL_GET_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class LocalGetResponse : public MessageBase
    {
    public:
        LocalGetResponse(const Key& key, const Value& value);
        LocalGetResponse(const DynamicArray& msg_payload);
        ~LocalGetResponse();

        Key getKey() const;
        Value getValue() const;

        virtual uint32_t getMsgPayloadSize() override;

    private:
        static const std::string kClassName;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) override;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position) override;

        Key key_;
        Value value_;
    };
}

#endif