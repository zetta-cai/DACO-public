#include "common/value.h"

#include <arpa/inet.h> // htonl ntohl

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const uint32_t Value::MAX_VALUE_CONTENT_SIZE = 0; // NOT transmit value content in network packets by default (just impl trick yet NOT affect evaluation results; see NOTE in header file)

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

    std::string Value::generateValuestrForStorage() const
    {
        return generateValuestr_(valuesize_);
    }

    uint32_t Value::getValuePayloadSize(const bool& is_space_efficient) const
    {
        // is deleted + value size
        uint32_t value_payload_size = sizeof(bool) + sizeof(uint32_t);
        // value
        if (!is_space_efficient) // For network packets
        {
            value_payload_size += getValuesizeForNetwork_(valuesize_);
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
        if (!is_space_efficient) // For network packets
        {
            std::string valuestr_for_network = generateValuestrForNetwork_();
            if (valuestr_for_network.length() > 0)
            {
                msg_payload.deserialize(size, (const char*)(valuestr_for_network.data()), valuestr_for_network.length());
                size += valuestr_for_network.length();
            }
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
        if (!is_space_efficient) // For network packets
        {
            // Note: we ignore the value content yet still consume space
            const uint32_t value_size_for_network = getValuesizeForNetwork_(valuesize_);
            // msg_payload.arraycpy(size, value_content, 0, value_size_for_network);
            size += value_size_for_network;
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
        assert(is_space_efficient); // Must be space efficient due to for file I/O (dataset/workload file for replayed traces)
        return size;
    }

    const Value& Value::operator=(const Value& other)
    {
        is_deleted_ = other.is_deleted_;
        valuesize_ = other.valuesize_;
        return *this;
    }

    std::string Value::generateValuestrForNetwork_() const
    {
        return generateValuestr_(getValuesizeForNetwork_(valuesize_));
    }

    uint32_t Value::getValuesizeForNetwork_(const uint32_t& value_size)
    {
        uint32_t value_size_for_network = 0;
        if (value_size > MAX_VALUE_CONTENT_SIZE)
        {
            value_size_for_network = MAX_VALUE_CONTENT_SIZE;
        }
        else
        {
            value_size_for_network = value_size;
        }
        assert(value_size_for_network <= MAX_VALUE_CONTENT_SIZE);

        return value_size_for_network;
    }

    std::string Value::generateValuestr_(const uint32_t& value_size)
    {
        std::string valuestr = "";
        if (value_size > 0)
        {
            if (Config::isGenerateRandomValuestr())
            {
                // Randomly generate characters to fill up value content
                valuestr = Util::getRandomString(value_size);
            }
            else
            {
                // Note: now we use '0' to fill up value content
                valuestr = std::string(value_size, '0');
            }
        }
        return valuestr;
    }
}