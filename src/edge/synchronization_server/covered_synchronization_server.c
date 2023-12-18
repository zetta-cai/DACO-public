#include "edge/synchronization_server/covered_synchronization_server.h"

#include <assert.h>

#include "common/config.h"

namespace covered
{
    const std::string CoveredSynchronizationServer::kClassName = "CoveredSynchronizationServer";

    void* CoveredSynchronizationServer::launchCoveredSynchronizationServer(void* synchronization_server_param_ptr)
    {
        assert(synchronization_server_param_ptr != NULL);

        CoveredSynchronizationServer synchronization_server((SynchronizationServerParam*)synchronization_server_param_ptr);
        synchronization_server.start();

        pthread_exit(NULL);
        return NULL;
    }

    CoveredSynchronizationServer::CoveredSynchronizationServer(SynchronizationServerParam* synchronization_server_param_ptr) : synchronization_server_param_ptr_(synchronization_server_param_ptr)
    {
        assert(synchronization_server_param_ptr != NULL);
        const uint32_t edge_idx = synchronization_server_param_ptr->getEdgeWrapperPtr()->getNodeIdx();
        //const uint32_t edgecnt = synchronization_server_param_ptr->getEdgeWrapperPtr()->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-synchronization-server";
        instance_name_ = oss.str();
    }

    CoveredSynchronizationServer::~CoveredSynchronizationServer()
    {
        // NOTE: no need to release synchronization_server_param_ptr_, which will be released outside CoveredSynchronizationServer (by EdgeWrapper)
    }

    void CoveredSynchronizationServer::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = synchronization_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the remote victim synchronization from ring buffer pushed by cache/beacon server
            SynchronizationServerItem tmp_synchronization_server_item;
            bool is_successful_for_remote_victim_synchronization = synchronization_server_param_ptr_->getSynchronizationServerItemBufferPtr()->pop(tmp_synchronization_server_item);
            if (is_successful_for_remote_victim_synchronization) // Receive an item for remote victim synchronization successfully
            {
                is_finish = processRemoteVictimSynchronization_(tmp_synchronization_server_item);

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful_for_remote_victim_synchronization == true)
        } // End of while loop

        return;
    }

    bool CoveredSynchronizationServer::processRemoteVictimSynchronization_(const SynchronizationServerItem& synchronization_server_item)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = synchronization_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Get required information from synchronization server item
        const uint32_t source_edge_idx = synchronization_server_item.getSourceEdgeIdx();
        const VictimSyncset& neighbor_victim_syncset = synchronization_server_item.getNeighborVictimSyncsetRef();

        // Victim synchronization
        // (OBSOLETE due to background processing) NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr()->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        return is_finish;
    }

    void CoveredSynchronizationServer::checkPointers_() const
    {
        assert(synchronization_server_param_ptr_ != NULL);

        return;
    }
}