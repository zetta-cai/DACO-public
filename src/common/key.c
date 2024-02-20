#include "common/key.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string Key::kClassName("Key");

    Key::Key()
    {
        keystr_ = "";
    }

    Key::Key(const std::string& keystr)
    {
        keystr_ = keystr;
    }

    Key::~Key() {}

    uint32_t Key::getKeyLength() const
    {
        return keystr_.length();
    }

    std::string Key::getKeystr() const
    {
        return keystr_;
    }

    std::string Key::getKeyIntstr() const
    {
        const uint32_t keylen = keystr_.length();
        assert(keylen == 4 || keylen == 8);

        std::string key_intstr = "";
        if (keylen == 4)
        {
            int32_t keyint = 0;
            memcpy(&keyint, keystr_.c_str(), sizeof(int32_t));
            key_intstr = std::to_string(keyint);
        }
        else
        {
            int64_t keyint = 0;
            memcpy(&keyint, keystr_.c_str(), sizeof(int64_t));
            key_intstr = std::to_string(keyint);
        }

        return key_intstr;
    }
    
    uint32_t Key::getKeyPayloadSize() const
    {
        // key size + key
        return sizeof(uint32_t) + keystr_.length();
    }

    uint32_t Key::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_keysize = htonl(keystr_.length());
        msg_payload.deserialize(size, (const char*)&bigendian_keysize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        msg_payload.deserialize(size, (const char*)(keystr_.data()), keystr_.length());
        size += keystr_.length();
        return size - position;
    }

    uint32_t Key::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t bigendian_keysize = 0;
        msg_payload.serialize(size, (char *)&bigendian_keysize, sizeof(uint32_t));
        uint32_t keysize = ntohl(bigendian_keysize);
        size += sizeof(uint32_t);
        DynamicArray keybytes(keysize);
        msg_payload.arraycpy(size, keybytes, 0, keysize);
        keystr_ = std::string(keybytes.getBytesRef().data(), keysize);
        size += keysize;
        return size - position;
    }

    bool Key::operator<(const Key& other) const
    {
        bool is_smaller = false;
        if (keystr_.compare(other.keystr_) < 0) // Current keystr length is smaller than other, or the first unmatched char is smaller than other
        {
            is_smaller = true;
        }
        return is_smaller;
    }

    bool Key::operator==(const Key& other) const
    {
        return keystr_ == other.keystr_;
    }

    bool Key::operator!=(const Key& other) const
    {
        return keystr_ != other.keystr_;
    }

    const Key& Key::operator=(const Key& other)
    {
        keystr_ = other.keystr_; // Deep copy
        return *this;
    }

    bool KeyHasher::equal(const Key& keya, const Key& keyb) const
    {
        return keya == keyb;
    }

    size_t KeyHasher::hash(const Key& key) const
    {
        return std::hash<std::string>{}(key.getKeystr());
    }

    size_t KeyHasher::operator()(const Key& key) const
    {
        return std::hash<std::string>{}(key.getKeystr());
    }
}