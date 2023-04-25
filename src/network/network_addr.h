/*
 * NetworkAddr: network address (including IP and port).
 *
 * Note: NetworkAddr can be used to locate fragment statistics of each message in MsgFragStats.
 * 
 * By Siyuan Sheng (2023.04.25).
 */

#ifndef NETWORK_ADDR_H
#define NETWORK_ADDR_H

#include <string>

namespace covered
{
    class NetworkAddr
    {
    public:
        NetworkAddr();
        NetworkAddr(const std::string& ipstr, const uint16_t& port, const bool& is_valid);
        NetworkAddr(const NetworkAddr& other);
        ~NetworkAddr();

        std::string getIpstr() const;
        void setIpstr(const std::string& ipstr);
        uint16_t getPort() const;
        void setPort(const uint16_t& port);
        bool isValid() const;
        void setValid();
        void resetValid();

        bool operator<(const NetworkAddr& other) const; // To be used as key in std::map
        NetworkAddr& operator=(const NetworkAddr& other); // assignment operation
    private:
        static const std::string kClassName;

        void checkPortIfValid_();

        std::string ipstr_;
        uint16_t port_;
        bool is_valid_;
    };
}



#endif