#include "common/value.h"

#include <arpa/inet.h> // htonl ntohl

#include "common/config.h"
#include "common/util.h"

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
        std::string valuestr = "";
        if (Config::isGenerateRandomValuestr())
        {
            // Randomly generate characters to fill up value content
            valuestr = Util::getRandomString(valuesize_);
        }
        else
        {
            // Note: now we use '0' to fill up value content
            valuestr = std::string(valuesize_, '0');
        }
        return valuestr;
    }

    uint32_t Value::getValuePayloadSize(const bool& is_space_efficient) const
    {
        // is deleted + value size
        uint32_t value_payload_size = sizeof(bool) + sizeof(uint32_t);
        // value
        if (!is_space_efficient)
        {
            value_payload_size += valuesize_;
        }
        return value_payload_size;
    }

    uint32_t Value::serialize(DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&is_deleted_, sizeof(bool));
        size += sizeof(bool);
        uint32_t bigendian_valuesize = htonl(valuesize_);
        msg_payload.deserialize(size, (const char*)&bigendian_valuesize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        if (!is_space_efficient)
        {
            std::string valuestr = generateValuestr();
            msg_payload.deserialize(size, (const char*)(valuestr.data()), valuesize_);
            size += valuesize_;
        }
        return size - position;
    }

    uint32_t Value::deserialize(const DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char *)&is_deleted_, sizeof(bool));
        size += sizeof(bool);
        uint32_t bigendian_valuesize = 0;
        msg_payload.serialize(size, (char *)&bigendian_valuesize, sizeof(uint32_t));
        valuesize_ = ntohl(bigendian_valuesize);
        size += sizeof(uint32_t);
        if (!is_space_efficient)
        {
            // Note: we ignore the value content yet still consume space
            // msg_payload.arraycpy(size, value_content, 0, valuesize_);
            size += valuesize_;
        }
        return size - position;
    }

    uint32_t Value::deserialize(std::fstream* fs_ptr, const bool& is_space_efficient)
    {
        uint32_t size = 0;
        fs_ptr->read((char*)&is_deleted_, sizeof(bool));
        size += sizeof(bool);
        uint32_t bitendian_valuesize = 0;
        fs_ptr->read((char*)&bitendian_valuesize, sizeof(uint32_t));
        valuesize_ = ntohl(bitendian_valuesize);
        size += sizeof(uint32_t);
        if (!is_space_efficient)
        {
            // Note: we ignore the value content
            // fs_ptr->read(value_content.getBytesRef().data(), valuesize_);
            size += valuesize_;
        }
        return size;
    }

    const Value& Value::operator=(const Value& other)
    {
        is_deleted_ = other.is_deleted_;
        valuesize_ = other.valuesize_;
        return *this;
    }
}