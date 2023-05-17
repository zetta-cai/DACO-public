/*
 * LocalPutRequest: a request issued by a client to insert/update a new value.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_PUT_REQUEST_H
#define LOCAL_PUT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class LocalPutRequest : public MessageBase
    {
    public:
        LocalPutRequest(const Key& key, const Value& value);
        LocalPutRequest(const DynamicArray& msg_payload);
        ~LocalPutRequest();

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