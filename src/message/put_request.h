/*
 * PutRequest: a request to insert/update a new value.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef PUT_REQUEST_H
#define PUT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"
#include "message/message_type.h"

namespace covered
{
    class PutRequest : public MessageBase
    {
    public:
        PutRequest(const Key& key, const Value& value);
        ~PutRequest();

        Key getKey();
        Value getValue();

        virtual uint32_t getMsgPayloadSize() override;

        // Offset of request must be 0 in message payload
        virtual uint32_t serialize(DynamicArray& msg_payload) override;
        virtual uint32_t deserialize(const DynamicArray& msg_payload) override;
    private:
        static const std::string kClassName;

        Key key_;
        Value value_;
    };
}

#endif