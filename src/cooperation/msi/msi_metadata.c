#include "cooperation/msi/msi_metadata.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
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

    // (1) Access write flag

    bool MsiMetadata::isBeingWritten() const
    {
        return writeflag_;
    }

    bool MsiMetadata::checkAndSetWriteflag()
    {
        bool is_successful = false;
        if (!writeflag_) // key is NOT being written
        {
            assert(edge_blocklist_.size() == 0);

            writeflag_ = true;
            is_successful = true;
        }
        return is_successful;
    }

    bool MsiMetadata::resetWriteflag()
    {
        bool original_writeflag = writeflag_;
        writeflag_ = false;
        return original_writeflag;
    }

    // (2) Access write flag and blocklist

    bool MsiMetadata::addEdgeIntoBlocklistIfBeingWritten(const NetworkAddr& network_addr, bool& is_being_written)
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

    // (3) For ConcurrentHashtable

    uint32_t MsiMetadata::getSizeForCapacity() const
    {
        uint32_t size = 0;
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

        if (function_name == "checkAndSetWriteflag")
        {
            CheckAndSetWriteflagParam* tmp_param_ptr = static_cast<CheckAndSetWriteflagParam*>(param_ptr);
            tmp_param_ptr->is_successful = checkAndSetWriteflag();
        }
        else if (function_name == "resetWriteflag")
        {
            ResetWriteflagParam* tmp_param_ptr = static_cast<ResetWriteflagParam*>(param_ptr);
            tmp_param_ptr->original_writeflag = resetWriteflag();
        }
        else if (function_name == "addEdgeIntoBlocklistIfBeingWritten")
        {
            AddEdgeIntoBlocklistIfBeingWrittenParam* tmp_param_ptr = static_cast<AddEdgeIntoBlocklistIfBeingWrittenParam*>(param_ptr);
            tmp_param_ptr->is_blocked_before = addEdgeIntoBlocklistIfBeingWritten(tmp_param_ptr->network_addr, tmp_param_ptr->is_being_written);
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

        if (function_name == "isBeingWritten")
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

    MsiMetadata& MsiMetadata::operator=(const MsiMetadata& other)
    {
        writeflag_ = other.writeflag_;
        edge_blocklist_ = other.edge_blocklist_;
        return *this;
    }
}