#include "cooperation/cooperation_wrapper_base.h"

#include <assert.h>
#include <sstream>
#include <unordered_map>

#include "common/param.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_wrapper.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CooperationWrapperBase::kClassName("CooperationWrapperBase");

    CooperationWrapperBase* CooperationWrapperBase::getCooperationWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr)
    {
        CooperationWrapperBase* cooperation_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            //cooperation_wrapper_ptr = new CoveredCooperationWrapper(hash_name, edge_param_ptr);
        }
        else
        {
            cooperation_wrapper_ptr = new BasicCooperationWrapper(hash_name, edge_param_ptr);
        }

        assert(cooperation_wrapper_ptr != NULL);
        return cooperation_wrapper_ptr;
    }

    CooperationWrapperBase::CooperationWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr) : block_tracker_(edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        base_instance_name_ = oss.str();

        dht_wrapper_ptr_ = new DhtWrapper(hash_name, edge_param_ptr);
        assert(dht_wrapper_ptr_ != NULL);

        // Allocate directory information
        uint32_t seed_for_directory_selection = edge_param_ptr->getEdgeIdx();
        directory_table_ptr_ = new DirectoryTable(seed_for_directory_selection, edge_param_ptr->getEdgeIdx());
        assert(directory_table_ptr_ != NULL);
    }

    CooperationWrapperBase::~CooperationWrapperBase()
    {
        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        // Release directory information
        assert(directory_table_ptr_ != NULL);
        delete directory_table_ptr_;
        directory_table_ptr_ = NULL;
    }

    // (1) Locate beacon edge node

    uint32_t CooperationWrapperBase::getBeaconEdgeIdx(const Key& key) const
    {
        assert(dht_wrapper_ptr_ != NULL);

        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        return beacon_edge_idx;
    }
        
    NetworkAddr CooperationWrapperBase::getBeaconEdgeBeaconServerAddr(const Key& key) const
    {
        std::string beacon_edge_ipstr = dht_wrapper_ptr_->getBeaconEdgeIpstr(key);
        uint16_t beacon_edge_beacon_server_port = dht_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqPort(key);
        NetworkAddr beacon_edge_beacon_server_addr(beacon_edge_ipstr, beacon_edge_beacon_server_port);
        return beacon_edge_beacon_server_addr;
    }

    // (2) Access content directory information

    void CooperationWrapperBase::lookupLocalDirectoryByCacheServer(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        is_being_written = block_tracker_.isBeingWrittenForKey(key);
        lookupLocalDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);
        return;
    }

    void CooperationWrapperBase::lookupLocalDirectoryByBeaconServer(const Key& key, const NetworkAddr& network_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        block_tracker_.blockEdgeForKeyIfExistAndBeingWritten(key, network_addr, is_being_written);
        lookupLocalDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);
        return;
    }

    void CooperationWrapperBase::lookupLocalDirectory_(const Key& key, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
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
        if (is_admit) // is_being_written affects validity of both directory info and cached object for cache admission
        {
            is_being_written = block_tracker_.isBeingWrittenForKey(key);
        }
        else // is_being_written does NOT affect cache eviction
        {
            is_being_written = false;
        }
        DirectoryMetadata directory_metadata(!is_being_written); // valid if not being written

        assert(directory_table_ptr_ != NULL);
        directory_table_ptr_->update(key, is_admit, directory_info, directory_metadata);
        return;
    }

    // (4) Process writes for MSI protocol

    LockResult CooperationWrapperBase::acquireLocalWritelockByCacheServer(const Key& key, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        assert(directory_table_ptr_ != NULL);

        LockResult lock_result = LockResult::kNoneed;

        // Check if key is cooperatively cached first
        bool is_cooperative_cached = directory_table_ptr_->isCooperativeCached(key);

        if (is_cooperative_cached) // Acquire write lock for cooperatively cached object for MSI protocol
        {
            bool is_successful = block_tracker_.casWriteflagForKey(key);
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

        return lock_result;
    }

    LockResult CooperationWrapperBase::acquireLocalWritelockByBeaconServer(const Key& key, const NetworkAddr& network_addr, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        assert(directory_table_ptr_ != NULL);

        LockResult lock_result = LockResult::kNoneed;

        // Check if key is cooperatively cached first
        bool is_cooperative_cached = directory_table_ptr_->isCooperativeCached(key);

        if (is_cooperative_cached) // Acquire write lock for cooperatively cached object for MSI protocol
        {
            bool is_successful = block_tracker_.casWriteflagOrBlockEdgeForKey(key, network_addr);
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

        return lock_result;
    }

    std::unordered_set<NetworkAddr, NetworkAddrHasher> CooperationWrapperBase::releaseLocalWritelock(const Key& key, const DirectoryInfo& sender_dirinfo)
    {
        assert(directory_table_ptr_ != NULL);

        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = block_tracker_.unblockAllEdgesAndFinishWriteForKeyIfExist(key);

        // Validate content directory if any for the closest edge node releasing the write lock
        directory_table_ptr_->validateDirinfoForKeyIfExist(key, sender_dirinfo);

        return blocked_edges;
    }

    // (5) Get size for capacity check

    uint32_t CooperationWrapperBase::getSizeForCapacity() const
    {
        assert(directory_table_ptr_ != NULL);
        uint32_t directory_table_size = directory_table_ptr_->getSizeForCapacity();
        uint32_t block_tracker_size = block_tracker_.getSizeForCapacity();
        return directory_table_size + block_tracker_size;
    }
}