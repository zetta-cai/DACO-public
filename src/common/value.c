#include "common/value.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string Value::kClassName("Value");

    Value::Value()
    {
        is_deleted_ = true;
        valuesize_ = 0;
    }

    Value::Value(const uint32_t& valuesize)
    {
        is_deleted_ = false;
        valuesize_ = valuesize;
    }

    Value::~Value() {}

    bool Value::isDeleted() const
    {
        return is_deleted_;
    }

    /*void Value::remove()
    {
        is_deleted_ = true;
        valuesize_ = 0;
    }*/

    uint32_t Value::getValuesize() const
    {
        return valuesize_;
    }

    std::string Value::generateValuestr() const
    {
        // Note: now we use '0' to fill up value content
        // TODO: we could randomly generate characters to fill up value content
        return std::string(valuesize_, '0');
    }

    uint32_t Value::serialize(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.write(size, (const char*)&is_deleted_, sizeof(bool));
        size += sizeof(bool);
        uint32_t bigendian_valuesize = htonl(valuesize_);
        msg_payload.write(size, (const char*)&bigendian_valuesize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        std::string valuestr = generateValuestr();
        msg_payload.write(size, (const char*)(valuestr.data()), valuesize_);
        size += valuesize_;
        return size;
    }

    uint32_t Value::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.read(size, (char *)&is_deleted_, sizeof(bool));
        size += sizeof(bool);
        uint32_t bigendian_valuesize = 0;
        msg_payload.read(size, (char *)&bigendian_valuesize, sizeof(uint32_t));
        valuesize_ = ntohl(bigendian_valuesize);
        size += sizeof(uint32_t);
        // Note: we ignore the value content
        // msg_payload.arraycpy(size, value_content, 0, valuesize_);
        size += valuesize_;
        return size;
    }

    Value& Value::operator=(const Value& other)
    {
        is_deleted_ = other.is_deleted_;
        valuesize_ = other.valuesize_;
        return *this;
    }
}