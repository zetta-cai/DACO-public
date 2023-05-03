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

    std::string Key::getKeystr()
    {
        return keystr_;
    }

    uint32_t Key::serialize(DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_keysize = htonl(keystr_.length());
        msg_payload.write(0, (const char*)&bigendian_keysize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        msg_payload.write(size, (const char*)(keystr_.data()), keystr_.length());
        size += keystr_.length();
        return size;
    }

    uint32_t Key::deserialize(const DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_keysize = 0;
        msg_payload.read(0, (char *)&bigendian_keysize, sizeof(uint32_t));
        uint32_t keysize = ntohl(bigendian_keysize);
        size += sizeof(uint32_t);
        char keycstr[keysize];
        msg_payload.read(size, (char *)keycstr, keysize);
        keystr_ = std::string(keycstr);
        size += keysize;
        return size;
    }
}