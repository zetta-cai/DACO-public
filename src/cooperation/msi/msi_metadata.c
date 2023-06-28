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

    MsiMetadata::~MsiMetadata() {}

    bool MsiMetadata::isBeingWritten() const
    {
        return writeflag_;
    }

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

        if (function_name == "isBeingWritten")
        {
            // TODO
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