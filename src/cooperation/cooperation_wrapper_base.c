#include "cooperation/cooperation_wrapper_base.h"

#include <assert.h>
#include <sstream>
#include <unordered_map>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_wrapper.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CooperationWrapperBase::kClassName("CooperationWrapperBase");

    CooperationWrapperBase* CooperationWrapperBase::getCooperationWrapperByCacheName(const std::string& cache_name, const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name)
    {
        CooperationWrapperBase* cooperation_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            //cooperation_wrapper_ptr = new CoveredCooperationWrapper(hash_name, edge_param_ptr);
        }
        else
        {
            cooperation_wrapper_ptr = new BasicCooperationWrapper(edgecnt, edge_idx, hash_name);
        }

        assert(cooperation_wrapper_ptr != NULL);
        return cooperation_wrapper_ptr;
    }

    CooperationWrapperBase::CooperationWrapperBase(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        dht_wrapper_ptr_ = new DhtWrapper(hash_name, edgecnt, edge_idx);
        assert(dht_wrapper_ptr_ != NULL);

        // Allocate per-key rwlock
        cooperation_wrapper_perkey_rwlock_ptr_ = new PerkeyRwlock(edge_idx, Config::getFineGrainedLockingSize());
        assert(cooperation_wrapper_perkey_rwlock_ptr_ != NULL);

        // Allocate directory information
        uint32_t seed_for_directory_selection = edge_idx;
        directory_table_ptr_ = new DirectoryTable(seed_for_directory_selection, edge_idx, cooperation_wrapper_perkey_rwlock_ptr_);
        assert(directory_table_ptr_ != NULL);

        // Allocate block tracker
        block_tracker_ptr_ = new BlockTracker(edge_idx, cooperation_wrapper_perkey_rwlock_ptr_);
        assert(block_tracker_ptr_ != NULL);
    }

    CooperationWrapperBase::~CooperationWrapperBase()
    {
        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        // Release per-key rwlock
        assert(cooperation_wrapper_perkey_rwlock_ptr_ != NULL);
        delete cooperation_wrapper_perkey_rwlock_ptr_;
        cooperation_wrapper_perkey_rwlock_ptr_ = NULL;

        // Release directory information
        assert(directory_table_ptr_ != NULL);
        delete directory_table_ptr_;
        directory_table_ptr_ = NULL;

        // Release block tracker
        assert(block_tracker_ptr_ != NULL);
        delete block_tracker_ptr_;
        block_tracker_ptr_ = NULL;
    }

    // (1) Locate beacon edge node

    uint32_t CooperationWrapperBase::getBeaconEdgeIdx(const Key& key) const
    {
        // No need to acquire a read lock due to accessing a const shared variable

        assert(dht_wrapper_ptr_ != NULL);

        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        return beacon_edge_idx;
    }
        
    NetworkAddr CooperationWrapperBase::getBeaconEdgeBeaconServerRecvreqAddr(const Key& key) const
    {
        // No need to acquire a read lock due to accessing a const shared variable

        std::string beacon_edge_ipstr = dht_wrapper_ptr_->getBeaconEdgeIpstr(key);
        uint16_t beacon_edge_beacon_server_recvreq_port = dht_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqPort(key);
        NetworkAddr beacon_edge_beacon_server_recvreq_addr(beacon_edge_ipstr, beacon_edge_beacon_server_recvreq_port);
        return beacon_edge_beacon_server_recvreq_addr;
    }

    // (2) Access content directory information

    void CooperationWrapperBase::lookupLocalDirectoryByCacheServer(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CooperationWrapperBase::lookupLocalDirectoryByCacheServer()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        is_being_written = block_tracker_ptr_->isBeingWrittenForKey(key);
        lookupLocalDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);

        // Release a read lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return;
    }

    void CooperationWrapperBase::lookupLocalDirectoryByBeaconServer(const Key& key, const NetworkAddr& cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info)
    {
        checkPointers_();

        // NOTE: we have to acquire a write lock as we may need to update the blocklist in BlockTracker
        std::string context_name = "CooperationWrapperBase::lookupLocalDirectoryByBeaconServer()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        block_tracker_ptr_->blockEdgeForKeyIfExistAndBeingWritten(key, cache_server_worker_recvreq_dst_addr, is_being_written);
        lookupLocalDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);

        // Release a read lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }

    void CooperationWrapperBase::lookupLocalDirectory_(const Key& key, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // No need to acquire a read/write lock, which has been done in public functions

        if (!is_being_written) // if key is NOT being written
        {
            assert(directory_table_ptr_ != NULL);
            directory_table_ptr_->lookup(key, is_valid_directory_exist, directory_info);
        }
        else // key is being written
        {
            is_valid_directory_exist = false;
            directory_info = DirectoryInfo();
        }
        return;
    }

    void CooperationWrapperBase::updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CooperationWrapperBase::updateLocalDirectory()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        if (is_admit) // is_being_written affects validity of both directory info and cached object for cache admission
        {
            is_being_written = block_tracker_ptr_->isBeingWrittenForKey(key);
        }
        else // is_being_written does NOT affect cache eviction
        {
            is_being_written = false;
        }
        DirectoryMetadata directory_metadata(!is_being_written); // valid if not being written

        assert(directory_table_ptr_ != NULL);
        directory_table_ptr_->update(key, is_admit, directory_info, directory_metadata);

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }

    // (4) Process writes for MSI protocol

    LockResult CooperationWrapperBase::acquireLocalWritelockByCacheServer(const Key& key, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CooperationWrapperBase::acquireLocalWritelockByCacheServer()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        LockResult lock_result = LockResult::kNoneed;

        // Check if key is cooperatively cached first
        bool is_cooperative_cached = directory_table_ptr_->isCooperativeCached(key);

        if (is_cooperative_cached) // Acquire write lock for cooperatively cached object for MSI protocol
        {
            bool is_successful = block_tracker_ptr_->casWriteflagForKey(key);
            if (is_successful) // Acquire write lock successfully
            {
                // Invalidate all content directory informations
                directory_table_ptr_->invalidateAllDirinfoForKeyIfExist(key, all_dirinfo);

                lock_result = LockResult::kSuccess;
            }
            else // NOT acquire write lock
            {
                lock_result = LockResult::kFailure;
            }
        }

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return lock_result;
    }

    LockResult CooperationWrapperBase::acquireLocalWritelockByBeaconServer(const Key& key, const NetworkAddr& cache_server_worker_recvreq_dst_addr, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CooperationWrapperBase::acquireLocalWritelockByBeaconServer()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        LockResult lock_result = LockResult::kNoneed;

        // Check if key is cooperatively cached first
        bool is_cooperative_cached = directory_table_ptr_->isCooperativeCached(key);

        if (is_cooperative_cached) // Acquire write lock for cooperatively cached object for MSI protocol
        {
            bool is_successful = block_tracker_ptr_->casWriteflagOrBlockEdgeForKey(key, cache_server_worker_recvreq_dst_addr);
            if (is_successful) // Acquire write lock successfully
            {
                // Invalidate all content directory informations
                directory_table_ptr_->invalidateAllDirinfoForKeyIfExist(key, all_dirinfo);

                lock_result = LockResult::kSuccess;
            }
            else // NOT acquire write lock
            {
                lock_result = LockResult::kFailure;
            }
        }

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return lock_result;
    }

    std::unordered_set<NetworkAddr, NetworkAddrHasher> CooperationWrapperBase::releaseLocalWritelock(const Key& key, const DirectoryInfo& sender_dirinfo)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CooperationWrapperBase::releaseLocalWritelock()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = block_tracker_ptr_->unblockAllEdgesAndFinishWriteForKeyIfExist(key);

        // Validate content directory if any for the closest edge node releasing the write lock
        directory_table_ptr_->validateDirinfoForKeyIfExist(key, sender_dirinfo);

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return blocked_edges;
    }

    // (5) Get size for capacity check

    uint64_t CooperationWrapperBase::getSizeForCapacity() const
    {
        checkPointers_();

        // No need to acquire a read lock due to no key provided and not critical for size

        uint64_t directory_table_size = directory_table_ptr_->getSizeForCapacity();
        uint64_t block_tracker_size = block_tracker_ptr_->getSizeForCapacity();
        return directory_table_size + block_tracker_size;
    }

    void CooperationWrapperBase::checkPointers_() const
    {
        assert(dht_wrapper_ptr_ != NULL);
        assert(cooperation_wrapper_perkey_rwlock_ptr_ != NULL);
        assert(directory_table_ptr_ != NULL);
        assert(block_tracker_ptr_ != NULL);
    }
}