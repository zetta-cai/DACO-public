/*
 * LocalGetRequest: a request issued by a client to get an existing value if any.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef LOCAL_GET_REQUEST_H
#define LOCAL_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "common/value.h"
#include "message/message_base.h"

namespace covered
{
    class LocalGetRequest : public MessageBase
    {
    public:
        LocalGetRequest(const Key& key);
        LocalGetRequest(const DynamicArray& msg_payload);
        ~LocalGetRequest();

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