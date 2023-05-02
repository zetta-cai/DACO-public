#include "network/network_addr.h"

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

    NetworkAddr::NetworkAddr(const std::string& ipstr, const uint16_t& port, const bool& is_valid)
    {
        ipstr_ = ipstr;
        port_ = port;
        is_valid_ = is_valid;
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

    bool NetworkAddr::isValid() const
    {
        return is_valid_;
    }

    void NetworkAddr::setValid()
    {
        is_valid_ = true;
        checkPortIfValid_();
        return;
    }

    void NetworkAddr::resetValid()
    {
        is_valid_ = false;
        return;
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

    void NetworkAddr::checkPortIfValid_()
    {
        // UDP port must be > Util::UDP_MAX_PORT
		if (is_valid_ && port_ <= Util::UDP_MAX_PORT)
		{
			std::ostringstream oss;
			oss << "invalid port of " << port_ << " which should be > " << Util::UDP_MAX_PORT << "!";	
			Util::dumpErrorMsg(kClassName, oss.str());
			exit(1);
		}
        return;
    }
}