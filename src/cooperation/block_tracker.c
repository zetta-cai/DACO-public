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

    BlockTracker::BlockTracker(EdgeParam* edge_param_ptr) : perkey_msimetadata_("perkey_msimetadata_", MsiMetadata())
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
    }

    BlockTracker::~BlockTracker() {}

    // (1) Access per-key write flag

    bool BlockTracker::isBeingWrittenForKey(const Key& key) const
    {
        // Prepare IsBeingWrittenParam
        MsiMetadata::IsBeingWrittenParam tmp_param = {false};

        bool is_exist = false;
        perkey_msimetadata_.constCallIfExist(key, is_exist, "isBeingWritten", &tmp_param);
        
        bool is_being_written = false;
        if (is_exist) // key exists
        {
            is_being_written = tmp_param.is_being_written;
        }

        return is_being_written;
    }

    bool BlockTracker::checkAndSetWriteflagForKey(const Key& key)
    {
        // Prepare an MSI metadata with writeflag_ = true
        MsiMetadata tmp_msimetadata;
        tmp_msimetadata.setWriteflag();

        // Prepare CheckAndSetWriteflagParam
        MsiMetadata::CheckAndSetWriteflagParam tmp_param = {false};

        bool is_exist = false;
        perkey_msimetadata_.insertOrCall(key, tmp_msimetadata, is_exist, "checkAndSetWriteflag", &tmp_param);

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

    void BlockTracker::resetWriteflagForKeyIfExist(const Key& key)
    {
        // Prepare ResetWriteflagParam
        MsiMetadata::ResetWriteflagParam tmp_param = {false};

        // Reset original write flag if key exists
        bool is_exist = false;
        perkey_msimetadata_.callIfExist(key, is_exist, "resetWriteflag", &tmp_param);

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
    }

    // (2) Access per-key write flag and blocklist

    void BlockTracker::blockEdgeForKeyIfExistAndBeingWritten(const Key& key, const NetworkAddr& network_addr, bool& is_being_written)
    {
        assert(network_addr.isValidAddr());

        // Prepare AddEdgeIntoBlocklistIfBeingWrittenParam
        bool is_blocked_before = false;
        MsiMetadata::AddEdgeIntoBlocklistIfBeingWrittenParam tmp_param = {network_addr, is_being_written, is_blocked_before};

        bool is_exist = false;
        perkey_msimetadata_.callIfExist(key, is_exist, "addEdgeIntoBlocklistIfBeingWritten", &tmp_param)

        if (!is_exist) // key does not exist
        {
            // No block list for key
            assert(!is_blocked_before);

            is_being_written = false;
        }
        else // key exists
        {
            // An edge node can be blocked at most once (i.e., no duplicate edge nodes in a blocklist)
            assert(!is_blocked_before);
        }

        return;
    }

    std::unordered_set<NetworkAddr, NetworkAddrHasher> BlockTracker::unblock(const Key& key)
    {
        // TODO: END HERE
        
        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_blockmeta_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_blockmeta_ptr_->try_lock("addEdgeIntoBlocklist()"))
            {
                break;
            }
        }

        // Double-check if key is being written again atomically
        bool is_being_written = false;
        std::unordered_map<Key, bool>::const_iterator iter = perkey_writeflags_.find(key);
        if (iter != perkey_writeflags_.end())
        {
            is_being_written = iter->second;
        }

        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges;
        if (!is_being_written) // key is still NOT being written
        {
            perkey_edge_blocklist_t::iterator iter = perkey_edge_blocklist_.find(key);
            assert(iter != perkey_edge_blocklist_.end()); // key must exist
            blocked_edges = iter->second;

            // Remove the key and block list from perkey_edge_blocklist_
            perkey_edge_blocklist_.erase(iter);
        }

        rwlock_for_blockmeta_ptr_->unlock();
        return blocked_edges;
    }

    // (3) Get size for capacity check

    uint32_t BlockTracker::getSizeForCapacity() const
    {
        // Acquire a read lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_blockmeta_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_blockmeta_ptr_->try_lock_shared("isBeingWritten()"))
            {
                break;
            }
        }

        // NOTE: we do NOT count key size in BlockTracker, as the size of keys managed by beacon edge node has been counted by DirectoryTable
        uint32_t size = 0;
        size += perkey_writeflags_.size() * sizeof(bool); // Size of write flags
        for (perkey_edge_blocklist_t::const_iterator iter_for_blocklist = perkey_edge_blocklist_.begin(); iter_for_blocklist != perkey_edge_blocklist_.end(); iter_for_blocklist++)
        {
            const std::unordered_set<NetworkAddr, NetworkAddrHasher>& tmp_edge_blocklist = iter_for_blocklist->second;
            for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_networkaddr = tmp_edge_blocklist.begin(); iter_for_networkaddr != tmp_edge_blocklist.end(); iter_for_networkaddr++)
            {
                size += iter_for_networkaddr->getSizeForCapacity(); // Size of network address
            }
        }

        rwlock_for_blockmeta_ptr_->unlock_shared();
        return size;
    }
}