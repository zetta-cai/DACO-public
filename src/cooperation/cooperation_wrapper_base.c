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

    void CooperationWrapperBase::lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        is_being_written = block_tracker_.isBeingWritten(key);
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
            is_being_written = block_tracker_.isBeingWritten(key);
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

    // (3) Access blocklist

    void CooperationWrapperBase::addEdgeIntoBlocklist(const Key& key, const NetworkAddr& network_addr)
    {
        block_tracker_.block(key, network_addr);
        return;
    }

    std::unordered_set<NetworkAddr, NetworkAddrHasher> CooperationWrapperBase::getBlocklistIfNoWrite(const Key& key)
    {
        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges;
        blocked_edges.clear();

        bool is_being_written = block_tracker_.isBeingWritten(key); // isBeingWritten_() acquires a read lock
        if (!is_being_written) // key is NOT being written
        {
            // Pop all blocked edge nodes
            blocked_edges = block_tracker_.unblock(key);
        }

        return blocked_edges;
    }

    // (4) Process writes for MSI protocol

    bool CooperationWrapperBase::acquireLocalWritelock(const Key& key, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        assert(directory_table_ptr_ != NULL);

        bool is_successful = block_tracker_.checkAndSetWriteflag(key);

        if (is_successful)
        {
            // Invalidate all content directory informations
            directory_table_ptr_->invalidateAllDirinfo(key, all_dirinfo);
        }

        return is_successful;
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