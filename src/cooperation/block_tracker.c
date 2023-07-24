#include "cooperation/block_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BlockTracker::kClassName("BlockTracker");

    BlockTracker::BlockTracker(const uint32_t& edge_idx, const PerkeyRwlock* perkey_rwlock_ptr) : perkey_msimetadata_("perkey_msimetadata_", MsiMetadata(), perkey_rwlock_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BlockTracker::~BlockTracker() {}

    // (1) For DirectoryLookup

    bool BlockTracker::isBeingWrittenForKey(const Key& key) const
    {
        // Prepare IsBeingWrittenParam
        MsiMetadata::IsBeingWrittenParam tmp_param = {false};

        bool is_exist = false;
        perkey_msimetadata_.constCallIfExist(key, is_exist, MsiMetadata::IS_BEING_WRITTEN_FUNCNAME, &tmp_param);
        
        bool is_being_written = false;
        if (is_exist) // key exists
        {
            is_being_written = tmp_param.is_being_written;
        }

        return is_being_written;
    }

    void BlockTracker::blockEdgeForKeyIfExistAndBeingWritten(const Key& key, const NetworkAddr& network_addr, bool& is_being_written)
    {
        assert(network_addr.isValidAddr());

        // Prepare BlockEdgeIfBeingWrittenParam
        MsiMetadata::BlockEdgeIfBeingWrittenParam tmp_param = {network_addr, false, false};

        bool is_exist = false;
        perkey_msimetadata_.callIfExist(key, is_exist, MsiMetadata::BLOCK_EDGE_IF_BEING_WRITTEN_FUNCNAME, &tmp_param);

        // An edge node can be blocked at most once (i.e., no duplicate edge nodes in a blocklist)
        assert(!tmp_param.is_blocked_before);

        if (!is_exist) // key does not exist
        {
            is_being_written = false;
        }
        else // key exists
        {
            is_being_written = tmp_param.is_being_written;
        }

        return;
    }

    // (2) For AcquireWritelock

    bool BlockTracker::casWriteflagForKey(const Key& key)
    {
        // Prepare an MSI metadata with writeflag_ = true
        MsiMetadata tmp_msimetadata(true);

        // Prepare CasWriteflagParam
        MsiMetadata::CasWriteflagParam tmp_param = {false};

        bool is_exist = false;
        perkey_msimetadata_.insertOrCall(key, tmp_msimetadata, is_exist, MsiMetadata::CAS_WRITEFLAG_FUNCNAME, &tmp_param);

        bool is_successful = false;
        if (!is_exist) // key NOT exist
        {
            is_successful = true;
        }
        else // key exists
        {
            is_successful = tmp_param.is_successful;
        }

        return is_successful;
    }

    bool BlockTracker::casWriteflagOrBlockEdgeForKey(const Key& key, const NetworkAddr& network_addr)
    {
        // Prepare an MSI metadata with writeflag_ = true
        MsiMetadata tmp_msimetadata(true);

        // Prepare CasWriteflagOrBlockEdgeParam
        MsiMetadata::CasWriteflagOrBlockEdgeParam tmp_param = {network_addr, false, false};

        bool is_exist = false;
        perkey_msimetadata_.insertOrCall(key, tmp_msimetadata, is_exist, MsiMetadata::CAS_WRITEFLAG_OR_BLOCK_EDGE_FUNCNAME, &tmp_param);
        assert(!tmp_param.is_blocked_before);

        bool is_successful = false;
        if (!is_exist) // key NOT exist
        {
            is_successful = true;
        }
        else // key exists
        {
            is_successful = tmp_param.is_successful;
        }

        return is_successful;
    }

    // (3) For FinishWrite

    /*void BlockTracker::resetWriteflagForKeyIfExist(const Key& key)
    {
        // Prepare ResetWriteflagParam
        MsiMetadata::ResetWriteflagParam tmp_param = {false};

        // Reset original write flag if key exists
        bool is_exist = false;
        perkey_msimetadata_.callIfExist(key, is_exist, MsiMetadata::RESET_WRITEFLAG_FUNCNAME, &tmp_param);

        if (!is_exist) // key does not exist
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does NOT exist in perkey_msimetadata_ for resetWriteflagForKeyIfExist()!";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        else // key exists
        {
            assert(tmp_param.original_writeflag == true); // key must be written before resetting writeflag
        }

        return;
    }*/

    std::unordered_set<NetworkAddr, NetworkAddrHasher> BlockTracker::unblockAllEdgesAndFinishWriteForKeyIfExist(const Key& key)
    {
        // Prepare UnblockAllEdgesAndFinishWriteParam
        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges;
        MsiMetadata::UnblockAllEdgesAndFinishWriteParam tmp_param = {blocked_edges};

        bool is_exist = false;
        perkey_msimetadata_.callIfExist(key, is_exist, MsiMetadata::UNBLOCK_ALL_EDGES_AND_FINISH_WRITE_FUNCNAME, &tmp_param);
        
        // key MUST exist
        assert(is_exist);

        return tmp_param.blocked_edges;
    }

    // (4) Get size for capacity check

    uint64_t BlockTracker::getSizeForCapacity() const
    {
        // NOTE: we do NOT count key size in BlockTracker, as the size of keys managed by beacon edge node has been counted by DirectoryTable
        uint64_t size = perkey_msimetadata_.getTotalValueSizeForCapcity();

        return size;
    }
}