#include "common/value.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string Value::kClassName("Value");

    Value::Value()
    {
        valuesize_ = 0;
    }

    Value::Value(const uint32_t& valuesize)
    {
        valuesize_ = valuesize;
    }

    Value::~Value() {}

    uint32_t Value::getValuesize() const
    {
        return valuesize_;
    }

    uint32_t Value::serialize(DynamicArray& msg_payload, uint32_t position)
    {
        uint32_t size = position;
        uint32_t bigendian_valuesize = htonl(valuesize_);
        msg_payload.write(size, (const char*)&bigendian_valuesize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // Note: we use 0 to fill up value content
        msg_payload.arrayset(size, 0, valuesize_);
        size += valuesize_;
        return size;
    }

    uint32_t Value::deserialize(const DynamicArray& msg_payload, uint32_t position)
    {
        uint32_t size = position;
        uint32_t bigendian_valuesize = 0;
        msg_payload.read(size, (char *)&bigendian_valuesize, sizeof(uint32_t));
        valuesize_ = ntohl(bigendian_valuesize);
        size += sizeof(uint32_t);
        // Note: we ignore the value content
        // msg_payload.arraycpy(size, value_content, 0, valuesize_);
        size += valuesize_;
        return size;
    }
}