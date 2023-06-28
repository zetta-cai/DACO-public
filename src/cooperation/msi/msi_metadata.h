/*
 * MsiMetadata: metadata for MSI protocol (building blocks of BlockTracker).
 * 
 * By Siyuan Sheng (2023.06.28).
 */

#ifndef MSI_METADATA_H
#define MSI_METADATA_H

#include <string>
#include <unordered_set>

#include "network/network_addr.h"

namespace covered
{
    typedef std::unordered_set<NetworkAddr, NetworkAddrHasher> edge_blocklist_t;

    class MsiMetadata
    {
    public:
        MsiMetadata();
        ~MsiMetadata();

        uint32_t getSizeForCapacity() const;
        void call(const std::string function_name, void* param_ptr);

        MsiMetadata& operator=(const MsiMetadata& other);
    private:
        bool writeflag_;
        edge_blocklist_t edge_blocklist_;
    };
}

#endif