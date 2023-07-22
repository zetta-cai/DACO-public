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

#include "common/dynamic_array.h"

namespace covered
{
    class NetworkAddr
    {
    public:
        NetworkAddr();
        NetworkAddr(const std::string& ipstr, const uint16_t& port);
        NetworkAddr(const NetworkAddr& other);
        ~NetworkAddr();

        std::string getIpstr() const;
        void setIpstr(const std::string& ipstr);
        uint16_t getPort() const;
        void setPort(const uint16_t& port);
        bool isValidAddr() const;
        void setValidAddr();
        void resetValidAddr();

        uint32_t getSizeForCapacity() const;

        uint32_t getAddrPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        std::string toString() const;

        bool operator<(const NetworkAddr& other) const; // To be used as network addr in std::map
        const NetworkAddr& operator=(const NetworkAddr& other); // assignment operation
        bool operator==(const NetworkAddr& other) const; // To be used by network addr in std::unordered_set
    private:
        static const std::string kClassName;

        void checkPortIfValid_();

        std::string ipstr_;
        uint16_t port_;
        
        bool is_valid_; // is_valid_ is just a boolean for assert and debug, which should NOT be counted into size
    };

    // To be used by network addr in std::unordered_set
    class NetworkAddrHasher
    {
    public:
        size_t operator()(const NetworkAddr& network_addr) const
        {
            size_t ip_hash = std::hash<std::string>{}(network_addr.getIpstr());
            size_t port_hash = std::hash<uint16_t>{}(network_addr.getPort());
            return ip_hash ^ port_hash;
        }
    };
}



#endif