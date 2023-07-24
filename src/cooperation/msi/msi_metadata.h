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

        struct BlockEdgeIfBeingWrittenParam
        {
            const NetworkAddr& network_addr;
            bool is_being_written;
            bool is_blocked_before;
        };

        struct CasWriteflagParam
        {
            bool is_successful;
        };

        struct CasWriteflagOrBlockEdgeParam
        {
            const NetworkAddr& network_addr;
            bool is_blocked_before;
            bool is_successful;
        };

        /*struct ResetWriteflagParam
        {
            bool original_writeflag;
        };*/

        struct UnblockAllEdgesAndFinishWriteParam
        {
            std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges;
        };

        static const std::string IS_BEING_WRITTEN_FUNCNAME;
        static const std::string BLOCK_EDGE_IF_BEING_WRITTEN_FUNCNAME;
        static const std::string CAS_WRITEFLAG_FUNCNAME;
        static const std::string CAS_WRITEFLAG_OR_BLOCK_EDGE_FUNCNAME;
        //static const std::string RESET_WRITEFLAG_FUNCNAME;
        static const std::string UNBLOCK_ALL_EDGES_AND_FINISH_WRITE_FUNCNAME;

        MsiMetadata();
        MsiMetadata(const bool& is_being_written);
        ~MsiMetadata();

        // (1) For DirectoryLookup

        bool isBeingWritten() const;
        // Return is blocked before
        bool blockEdgeIfBeingWritten(const NetworkAddr& network_addr, bool& is_being_written); // Add edge into blocklist if key is being written

        // (2) For AcquireWritelock

        bool casWriteflag(); // Check and set write flag
        bool casWriteflagOrBlockEdge(const NetworkAddr& network_addr, bool& is_blocked_before); // Check and set write flag, and block edge if fail to CAS write flag
        
        // (3) For FinishWrite

        //bool resetWriteflag(); // Return original writeflag_
        void unblockAllEdgesAndFinishWrite(std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges); // Remove edges from blocklist and finish write (key MUST being written)

        // (4) For ConcurrentHashtable

        uint64_t getSizeForCapacity() const;
        bool call(const std::string function_name, void* param_ptr);
        void constCall(const std::string function_name, void* param_ptr) const;

        const MsiMetadata& operator=(const MsiMetadata& other);
    private:
        static const std::string kClassName;

        bool writeflag_; // whether key is being written
        edge_blocklist_t edge_blocklist_; // a list of blocked closest edge nodes waiting for writes of each given key
    };
}

#endif