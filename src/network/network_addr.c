#include "network/network_addr.h"

#include <arpa/inet.h> // htonl ntohl
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string NetworkAddr::kClassName("NetworkAddr");

    NetworkAddr::NetworkAddr()
    {
        ipstr_ = "";
        port_ = 0;
        is_valid_ = false;
    }

    NetworkAddr::NetworkAddr(const std::string& ipstr, const uint16_t& port)
    {
        ipstr_ = ipstr;
        port_ = port;
        is_valid_ = true;
        checkPortIfValid_();
    }

    NetworkAddr::NetworkAddr(const NetworkAddr& other)
    {
        *this = other;
    }

    NetworkAddr::~NetworkAddr() {}

    std::string NetworkAddr::getIpstr() const
    {
        return ipstr_;
    }

    void NetworkAddr::setIpstr(const std::string& ipstr)
    {
        ipstr_ = ipstr;
        return;
    }

    uint16_t NetworkAddr::getPort() const
    {
        return port_;
    }

    void NetworkAddr::setPort(const uint16_t& port)
    {
        port_ = port;
        checkPortIfValid_();
        return;
    }

    bool NetworkAddr::isValidAddr() const
    {
        return is_valid_;
    }

    void NetworkAddr::setValidAddr()
    {
        is_valid_ = true;
        checkPortIfValid_();
        return;
    }

    void NetworkAddr::resetValidAddr()
    {
        is_valid_ = false;
        return;
    }

    uint32_t NetworkAddr::getSizeForCapacity() const
    {
        uint32_t size = ipstr_.length() + sizeof(uint16_t);
        return size;
    }

    uint32_t NetworkAddr::getAddrPayloadSize() const
    {
        // ipstr length + ipstr + port
        return sizeof(uint32_t) + ipstr_.length() + sizeof(uint16_t);
    }

    uint32_t NetworkAddr::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_ipstrsize = htonl(ipstr_.length());
        msg_payload.deserialize(size, (const char*)&bigendian_ipstrsize, sizeof(uint32_t));
        size += sizeof(uint32_t);
        msg_payload.deserialize(size, (const char*)ipstr_.data(), ipstr_.length());
        size += ipstr_.length();
        uint16_t bigendian_port = htons(port_);
        msg_payload.deserialize(size, (const char*)&bigendian_port, sizeof(uint16_t));
        size += sizeof(uint16_t);
        return size - position;
    }

    uint32_t NetworkAddr::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t bigendian_ipstrsize = 0;
        msg_payload.serialize(size, (char *)&bigendian_ipstrsize, sizeof(uint32_t));
        uint32_t ipstr_size = ntohl(bigendian_ipstrsize);
        size += sizeof(uint32_t);
        DynamicArray ipstr_bytes(ipstr_size);
        msg_payload.arraycpy(size, ipstr_bytes, 0, ipstr_size);
        ipstr_ = std::string(ipstr_bytes.getBytes().data(), ipstr_size);
        size += ipstr_size;
        uint16_t bigendian_port = 0;
        msg_payload.serialize(size, (char *)&bigendian_port, sizeof(uint16_t));
        port_ = ntohs(bigendian_port);
        size += sizeof(uint16_t);
        return size - position;
    }

    bool NetworkAddr::operator<(const NetworkAddr& other) const
    {
        bool is_smaller = false;
        if (ipstr_.compare(other.ipstr_)) // Matched char is smaller, or all chars are matched yet with smaller string length
        {
            is_smaller = true;
        }
        else if (port_ < other.port_)
        {
            is_smaller = true;
        }
        return is_smaller;
    }

    NetworkAddr& NetworkAddr::operator=(const NetworkAddr& other)
    {
        ipstr_ = other.ipstr_;
        port_ = other.port_;
        is_valid_ = other.is_valid_;
        checkPortIfValid_();
        return *this;
    }

    bool NetworkAddr::operator==(const NetworkAddr& other) const
    {
        // No need to compare is_valid_
        return ipstr_ == other.ipstr_ && port_ == other.port_;
    }

    void NetworkAddr::checkPortIfValid_()
    {
        // UDP port must be > Util::UDP_MIN_PORT
		if (is_valid_ && port_ <= Util::UDP_MIN_PORT)
		{
			std::ostringstream oss;
			oss << "invalid port of " << port_ << " which should be >= " << Util::UDP_MIN_PORT << "!";	
			Util::dumpErrorMsg(kClassName, oss.str());
			exit(1);
		}
        return;
    }
}