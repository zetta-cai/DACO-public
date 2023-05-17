/*
 * LocalPutResponse: a response issued by an edge node to a client for LocalPutRequest.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_PUT_RESPONSE_H
#define LOCAL_PUT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class LocalPutResponse : public MessageBase
    {
    public:
        LocalPutResponse(const Key& key);
        LocalPutResponse(const DynamicArray& msg_payload);
        ~LocalPutResponse();

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