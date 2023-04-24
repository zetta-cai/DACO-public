#include "network/udp_pkt_result.h"

#include "common/util.h"

namespace covered
{
    const std::string UdpPktResult::kClassName("UdpPktResult");

    UdpPktResult::UdpPktResult()
    {
        is_timeout_ = false;
        pkt_payload_.resize(0);
        pkt_payload_.reserve(Util::UDP_MAX_PKT_PAYLOAD);
        remote_ipstr_ = "";
        remote_port_ = 0;
    }

    UdpPktResult::~UdpPktResult() {}

    bool UdpPktResult::getIsTimeout()
    {
        return is_timeout_;
    }

    void UdpPktResult::setIsTimeout()
    {
        is_timeout_ = true;
        return;
    }

    void UdpPktResult::resetIsTimeout()
    {
        is_timeout_ = false;
        return;
    }

    std::vector<char>& UdpPktResult::getPktPayloadRef()
    {
        return pkt_payload_;
    }

    std::string UdpPktResult::getRemoteIpstr()
    {
        return remote_ipstr_;
    }

    void UdpPktResult::setRemoteIpstr(const std::string& remote_ipstr)
    {
        remote_ipstr_ = remote_ipstr;
        return;
    }

    uint16_t UdpPktResult::getRemotePort()
    {
        return remote_port_;
    }
    
    void UdpPktResult::setRemotePort(const uint16_t& remote_port)
    {
        if (remote_port <= Util::UDP_MAX_PORT)
        {
            std::ostringstream oss;
            oss << "invalid remote port of " << remote_port << " which should be > " << Util::UDP_MAX_PORT << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        remote_port_ = remote_port;
        return;
    }
}