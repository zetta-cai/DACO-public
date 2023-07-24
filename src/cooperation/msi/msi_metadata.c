#include "cooperation/msi/msi_metadata.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string MsiMetadata::IS_BEING_WRITTEN_FUNCNAME("isBeingWritten");
    const std::string MsiMetadata::BLOCK_EDGE_IF_BEING_WRITTEN_FUNCNAME("blockEdgeIfBeingWritten");
    const std::string MsiMetadata::CAS_WRITEFLAG_FUNCNAME("casWriteflag");
    const std::string MsiMetadata::CAS_WRITEFLAG_OR_BLOCK_EDGE_FUNCNAME("casWriteflagOrBlockEdge");
    //const std::string RESET_WRITEFLAG_FUNCNAME("resetWriteflag");
    const std::string MsiMetadata::UNBLOCK_ALL_EDGES_AND_FINISH_WRITE_FUNCNAME("unblockAllEdgesAndFinishWrite");

    const std::string MsiMetadata::kClassName("MsiMetadata");

    MsiMetadata::MsiMetadata()
    {
        writeflag_ = false;
        edge_blocklist_.clear();
    }

    MsiMetadata::MsiMetadata(const bool& is_being_written)
    {
        writeflag_ = is_being_written;
        edge_blocklist_.clear();
    }

    MsiMetadata::~MsiMetadata() {}

    // (1) For DirectoryLookup

    bool MsiMetadata::isBeingWritten() const
    {
        return writeflag_;
    }

    bool MsiMetadata::blockEdgeIfBeingWritten(const NetworkAddr& network_addr, bool& is_being_written)
    {
        assert(network_addr.isValidAddr());
        
        bool is_blocked_before = false;

        if (writeflag_) // key is being written
        {
            edge_blocklist_t::iterator iter = edge_blocklist_.find(network_addr);
            if (iter == edge_blocklist_.end()) // edge NOT blocked before
            {
                edge_blocklist_.insert(network_addr);
            }
            else // edge is blocked before
            {
                is_blocked_before = true;
            }

            is_being_written = true;
        }
        else // key is NOT being written
        {
            assert(edge_blocklist_.size() == 0);

            is_being_written = false;
        }

        return is_blocked_before;
    }

    // (2) For AcquireWritelock

    bool MsiMetadata::casWriteflag()
    {
        // Check and set write flag
        bool is_successful = false;
        if (!writeflag_) // key is NOT being written
        {
            assert(edge_blocklist_.size() == 0);

            writeflag_ = true;
            is_successful = true;
        }

        return is_successful;
    }

    bool MsiMetadata::casWriteflagOrBlockEdge(const NetworkAddr& network_addr, bool& is_blocked_before)
    {
        // Check and set write flag
        bool is_successful = casWriteflag();

        if (!is_successful) // key is already being written before CAS
        {
            // Add the edge into blocklist
            bool is_being_written = false;
            bool is_blocked_before = blockEdgeIfBeingWritten(network_addr, is_being_written);

            assert(!is_blocked_before);
            assert(is_being_written); // key MUST being written after CS
        }

        return is_successful;
    }

    // (3) For FinishWrite

    /*bool MsiMetadata::resetWriteflag()
    {
        bool original_writeflag = writeflag_;
        writeflag_ = false;
        return original_writeflag;
    }*/

    void MsiMetadata::unblockAllEdgesAndFinishWrite(std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges)
    {
        // key MUST being written
        assert(writeflag_);

        blocked_edges = edge_blocklist_; // Unblock all edges
        writeflag_ = false; // Finish write

        return;
    }

    // (4) For ConcurrentHashtable

    uint64_t MsiMetadata::getSizeForCapacity() const
    {
        uint64_t size = 0;
        size += sizeof(bool);
        for (edge_blocklist_t::const_iterator iter = edge_blocklist_.begin(); iter != edge_blocklist_.end(); iter++)
        {
            size += iter->getSizeForCapacity();
        }
        return size;
    }

    bool MsiMetadata::call(const std::string function_name, void* param_ptr)
    {
        assert(param_ptr != NULL);

        bool is_erase = false;

        if (function_name == CAS_WRITEFLAG_FUNCNAME)
        {
            CasWriteflagParam* tmp_param_ptr = static_cast<CasWriteflagParam*>(param_ptr);
            tmp_param_ptr->is_successful = casWriteflag();
        }
        else if (function_name == CAS_WRITEFLAG_OR_BLOCK_EDGE_FUNCNAME)
        {
            CasWriteflagOrBlockEdgeParam* tmp_param_ptr = static_cast<CasWriteflagOrBlockEdgeParam*>(param_ptr);
            tmp_param_ptr->is_successful = casWriteflagOrBlockEdge(tmp_param_ptr->network_addr, tmp_param_ptr->is_blocked_before);
        }
        /*else if (function_name == RESET_WRITEFLAG_FUNCNAME)
        {
            ResetWriteflagParam* tmp_param_ptr = static_cast<ResetWriteflagParam*>(param_ptr);
            tmp_param_ptr->original_writeflag = resetWriteflag();
        }*/
        else if (function_name == BLOCK_EDGE_IF_BEING_WRITTEN_FUNCNAME)
        {
            BlockEdgeIfBeingWrittenParam* tmp_param_ptr = static_cast<BlockEdgeIfBeingWrittenParam*>(param_ptr);
            tmp_param_ptr->is_blocked_before = blockEdgeIfBeingWritten(tmp_param_ptr->network_addr, tmp_param_ptr->is_being_written);
        }
        else if (function_name == UNBLOCK_ALL_EDGES_AND_FINISH_WRITE_FUNCNAME)
        {
            UnblockAllEdgesAndFinishWriteParam* tmp_param_ptr = static_cast<UnblockAllEdgesAndFinishWriteParam*>(param_ptr);
            unblockAllEdgesAndFinishWrite(tmp_param_ptr->blocked_edges);

            is_erase = true;
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for call()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return is_erase;
    }

    void MsiMetadata::constCall(const std::string function_name, void* param_ptr) const
    {
        assert(param_ptr != NULL);

        if (function_name == IS_BEING_WRITTEN_FUNCNAME)
        {
            IsBeingWrittenParam* tmp_param_ptr = static_cast<IsBeingWrittenParam*>(param_ptr);
            tmp_param_ptr->is_being_written = isBeingWritten();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for call()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }

    const MsiMetadata& MsiMetadata::operator=(const MsiMetadata& other)
    {
        writeflag_ = other.writeflag_;
        edge_blocklist_ = other.edge_blocklist_;
        return *this;
    }
}