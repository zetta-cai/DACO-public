/*
 * UdpPktResult: result of receiving a single UDP packet, including timeout flag, UDP payload, and remote address.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_PKT_RESULT_H
#define UDP_PKT_RESULT_H

#include <string>
#include <vector>

namespace covered
{
    class UdpPktResult
    {
    public:
        UdpPktResult();
        ~UdpPktResult();

        bool getIsTimeout();
        void setIsTimeout();
        void resetIsTimeout();
        std::vector<char>& getPktPayloadRef(); // Note: get reference to avoid unnecessary memory copy
        std::string getRemoteIpstr();
        void setRemoteIpstr(const std::string& remote_ipstr);
        uint16_t getRemotePort();
        void setRemotePort(const uint16_t& remote_port);

    private:
        static const std::string kClassName;

        bool is_timeout_;
        std::vector<char> pkt_payload_; // payload of a single UDP packet
        std::string remote_ipstr_;
        uint16_t remote_port_;
    };
}

#endif