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
        struct IsBeingWrittenParam
        {
            bool is_being_written;
        };

        MsiMetadata();
        ~MsiMetadata();

        bool isBeingWritten() const;

        uint32_t getSizeForCapacity() const;
        bool call(const std::string function_name, void* param_ptr);
        void constCall(const std::string function_name, void* param_ptr) const;

        MsiMetadata& operator=(const MsiMetadata& other);
    private:
        static const std::string kClassName;

        bool writeflag_; // whether key is being written
        edge_blocklist_t edge_blocklist_; // a list of blocked closest edge nodes waiting for writes of each given key
    };
}

#endif