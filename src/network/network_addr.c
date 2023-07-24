#include "network/network_addr.h"

#include <arpa/inet.h> // htonl ntohl inet_ntop inet_pton
#include <assert.h>
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

    uint64_t NetworkAddr::getSizeForCapacity() const
    {
        uint64_t size = static_cast<uint64_t>(ipstr_.length()) + sizeof(uint16_t);
        return size;
    }

    uint32_t NetworkAddr::getAddrPayloadSize() const
    {
        // 4B ipint + 2B port
        return sizeof(uint32_t) + sizeof(uint16_t);
    }

    uint32_t NetworkAddr::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        struct in_addr ipint;
        int return_val = inet_pton(AF_INET, ipstr_.c_str(), &ipint);
        assert(return_val >= 0);
        msg_payload.deserialize(size, (const char*)&(ipint.s_addr), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint16_t bigendian_port = htons(port_);
        msg_payload.deserialize(size, (const char*)&bigendian_port, sizeof(uint16_t));
        size += sizeof(uint16_t);
        return size - position;
    }

    uint32_t NetworkAddr::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        struct in_addr ipint;
        msg_payload.serialize(size, (char *)&(ipint.s_addr), sizeof(uint32_t));
        char ipstr_bytes[INET_ADDRSTRLEN]; // NOT use INET6_ADDRSTRLEN
        const char* result_ptr = inet_ntop(AF_INET, &ipint, ipstr_bytes, INET_ADDRSTRLEN);
        assert(result_ptr != NULL);
        ipstr_ = std::string(ipstr_bytes);
        size += sizeof(uint32_t);
        uint16_t bigendian_port = 0;
        msg_payload.serialize(size, (char *)&bigendian_port, sizeof(uint16_t));
        port_ = ntohs(bigendian_port);
        size += sizeof(uint16_t);

        is_valid_ = true;
        checkPortIfValid_();

        return size - position;
    }

    std::string NetworkAddr::toString() const
    {
        std::ostringstream oss;
        oss << "<" << ipstr_ << ", " << port_ << ">";
        return oss.str();
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

    const NetworkAddr& NetworkAddr::operator=(const NetworkAddr& other)
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