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

    BlockTracker::BlockTracker(EdgeParam* edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();

        edge_param_ptr_ = edge_param_ptr;
        assert(edge_param_ptr_ != NULL);

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_blockmeta_ptr_";
        rwlock_for_blockmeta_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_blockmeta_ptr_ != NULL);

        perkey_writeflags_.clear();
        perkey_edge_blocklist_.clear();
    }

    BlockTracker::~BlockTracker()
    {
        // NOTE: no need to release edge_param_ptr_, which will be released outside BlockTracker (e.g., simulator)

        assert(rwlock_for_blockmeta_ptr_ != NULL);
        delete rwlock_for_blockmeta_ptr_;
        rwlock_for_blockmeta_ptr_ = NULL;
    }

    // (1) Access per-key write flag

    bool BlockTracker::isBeingWritten(const Key& key) const
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

        bool is_being_written = false;
        std::unordered_map<Key, bool>::const_iterator iter = perkey_writeflags_.find(key);
        if (iter != perkey_writeflags_.end())
        {
            is_being_written = iter->second;
        }

        rwlock_for_blockmeta_ptr_->unlock_shared();
        return is_being_written;
    }

    bool BlockTracker::checkAndSetWriteflag(const Key& key)
    {
        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_blockmeta_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_blockmeta_ptr_->try_lock("tryToLockForWrite()"))
            {
                break;
            }
        }

        // Get original write flag
        bool is_being_written = false;
        std::unordered_map<Key, bool>::iterator iter = perkey_writeflags_.find(key);
        if (iter != perkey_writeflags_.end())
        {
            is_being_written = iter->second;
        }

        bool is_successful = false;
        if (!is_being_written) // Write permission has not been assigned
        {
            if (iter != perkey_writeflags_.end()) // key not exist
            {
                iter->second = true; // Assign write permission
            }
            else // key exists
            {
                perkey_writeflags_.insert(std::pair<Key, bool>(key, true));
            }
            is_successful = true;
        }

        rwlock_for_blockmeta_ptr_->unlock();
        return is_successful;
    }

    void BlockTracker::resetWriteflag(const Key& key)
    {
        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_blockmeta_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_blockmeta_ptr_->try_lock("resetWriteflag()"))
            {
                break;
            }
        }

        // Reset original write flag
        std::unordered_map<Key, bool>::iterator iter = perkey_writeflags_.find(key);
        if (iter == perkey_writeflags_.end()) // key does not exist
        {
            std::ostringstream oss;
            oss << "writeflag for key " << key.getKeystr() << " does NOT exist in perkey_writeflags_!";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        else // key exists
        {
            assert(iter->second == true); // existing key must be written
            iter->second = false;
            perkey_writeflags_.erase(iter); // key NOT exist == key NOT being written
        }

        rwlock_for_blockmeta_ptr_->unlock();
        return;
    }

    // (2) Access per-key blocklist

    void BlockTracker::block(const Key& key, const NetworkAddr& network_addr)
    {
        assert(network_addr.isValidAddr());

        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_blockmeta_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_blockmeta_ptr_->try_lock("addEdgeIntoBlocklist()"))
            {
                break;
            }
        }

        perkey_edge_blocklist_t::iterator iter = perkey_edge_blocklist_.find(key);
        if (iter != perkey_edge_blocklist_.end()) // key does not exist
        {
            std::unordered_set<NetworkAddr, NetworkAddrHasher> tmp_edge_blocklist;
            perkey_edge_blocklist_.insert(std::pair<Key, std::unordered_set<NetworkAddr, NetworkAddrHasher>>(key, tmp_edge_blocklist));
        }

        iter = perkey_edge_blocklist_.find(key);
        assert(iter != perkey_edge_blocklist_.end()); // key must exist now
        
        assert(iter->second.find(network_addr) == iter->second.end()); // An edge node can be blocked at most once (i.e., no duplicate edge nodes in a blocklist)
        iter->second.insert(network_addr);

        rwlock_for_blockmeta_ptr_->unlock();
        return;
    }

    std::unordered_set<NetworkAddr, NetworkAddrHasher> BlockTracker::unblock(const Key& key)
    {
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