#include "statistics/client_statistics_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/dynamic_array.h"
#include "common/util.h"

namespace covered
{
    const uint32_t ClientStatisticsTracker::CURSLOT_CLIENT_RAW_STATISTICS_CNT = 2;

    const std::string ClientStatisticsTracker::kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx, const uint32_t& curslot_client_raw_statistics_cnt) : perclient_workercnt_(perclient_workercnt)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // (B) Non-const individual variables

        // (B.1) For cur-slot client raw statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        perclientworker_curslot_update_flags_ = new std::atomic<bool>[perclient_workercnt_];
        assert(perclientworker_curslot_update_flags_ != NULL);
        Util::initializeAtomicArray<bool>(perclientworker_curslot_update_flags_, perclient_workercnt_, false);

        perclientworker_curslot_update_statuses_ = new std::atomic<uint64_t>[perclient_workercnt];
        assert(perclientworker_curslot_update_statuses_ != NULL);
        Util::initializeAtomicArray<uint64_t>(perclientworker_curslot_update_statuses_, perclient_workercnt, 0);

        curslot_client_raw_statistics_ptr_list_.resize(curslot_client_raw_statistics_cnt);
        for (uint32_t i = 0; i < curslot_client_raw_statistics_ptr_list_.size(); i++)
        {
            curslot_client_raw_statistics_ptr_list_[i] = new ClientRawStatistics(perclient_workercnt);
            assert(curslot_client_raw_statistics_ptr_list_[i] != NULL);
        }

        // (B.2) For stable client raw statistics

        stable_client_raw_statistics_ptr_ = new ClientRawStatistics(perclient_workercnt);
        assert(stable_client_raw_statistics_ptr_ != NULL);
    }

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        assert(perclientworker_curslot_update_flags_ != NULL);
        delete[] perclientworker_curslot_update_flags_;
        perclientworker_curslot_update_flags_ = NULL;

        assert(perclientworker_curslot_update_statuses_ != NULL);
        delete[] perclientworker_curslot_update_statuses_;
        perclientworker_curslot_update_statuses_ = NULL;

        assert(curslot_client_raw_statistics_ptr_list_.size() > 0);
        for (uint32_t i = 0; i < curslot_client_raw_statistics_ptr_list_.size(); i++)
        {
            assert(curslot_client_raw_statistics_ptr_list_[i] != NULL);
            delete curslot_client_raw_statistics_ptr_list_[i];
            curslot_client_raw_statistics_ptr_list_[i] = NULL;
        }

        assert(stable_client_raw_statistics_ptr_ != NULL);
        delete stable_client_raw_statistics_ptr_;
        stable_client_raw_statistics_ptr_ = NULL;
    }

    // (0) Get cur-slot/stable client raw statistics for debug (invoked by client workers)

    uint32_t ClientStatisticsTracker::getCurslotIdx() const
    {
        checkPointers_();

        return cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER);
    }
        
    uint32_t ClientStatisticsTracker::getCurslotReqcnt(const uint32_t& local_client_worker_idx) const
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        uint32_t curslot_reqcnt = tmp_curslot_client_raw_statistics_ptr->getReqcnt(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        return curslot_reqcnt;
    }

    // uint32_t ClientStatisticsTracker::getCurslotMaxLatency(const uint32_t& local_client_worker_idx) const
    // {
    //     checkPointers_();

    //     perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

    //     ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
    //     assert(tmp_curslot_client_raw_statistics_ptr != NULL);
    //     uint32_t max_latency_us = tmp_curslot_client_raw_statistics_ptr->getMaxlatency_();

    //     perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
    //     perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

    //     return max_latency_us;
    // }

    // (1) Update cur-slot/stable client raw statistics (invoked by client workers)

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateLocalHitcnt_(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateLocalHitcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateCooperativeHitcnt_(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateCooperativeHitcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateReqcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateReqcnt_(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateReqcnt_(local_client_worker_idx);
        }
        
        return;
    }

    void ClientStatisticsTracker::updateLocalHitbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateLocalHitbytes_(local_client_worker_idx, object_size);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateLocalHitbytes_(local_client_worker_idx, object_size);
        }

        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);
        
        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER)); 
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateCooperativeHitbytes_(local_client_worker_idx, object_size);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;
        
        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateCooperativeHitbytes_(local_client_worker_idx, object_size);
        }

        return;
    }

    void ClientStatisticsTracker::updateReqbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);
        
        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER)); 
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateReqbytes_(local_client_worker_idx, object_size);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;
        
        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateReqbytes_(local_client_worker_idx, object_size);
        }

        return;
    }

    void ClientStatisticsTracker::updateLatency(const uint32_t& local_client_worker_idx, const uint32_t& latency_us, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateLatency_(latency_us);


        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateLatency_(latency_us);
        }
        
        return;
    }

    void ClientStatisticsTracker::updateReadcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateReadcnt_(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateReadcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateWritecnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateWritecnt_(local_client_worker_idx);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateWritecnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateCacheUtilization(const uint32_t& local_client_worker_idx, const uint64_t& closest_edge_cache_size_bytes, const uint64_t& closest_edge_cache_capacity_bytes, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateCacheUtilization_(closest_edge_cache_size_bytes, closest_edge_cache_capacity_bytes);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateCacheUtilization_(closest_edge_cache_size_bytes, closest_edge_cache_capacity_bytes);
        }

        return;
    }

    // Update cur-slot/stable client raw statistics for value size
    
    void ClientStatisticsTracker::updateWorkloadKeyValueSize(const uint32_t& local_client_worker_idx, const uint32_t& key_size, const uint32_t& value_size, const Hitflag& hitflag, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateWorkloadKeyValueSize_(local_client_worker_idx, key_size, value_size, hitflag);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateWorkloadKeyValueSize_(local_client_worker_idx, key_size, value_size, hitflag);
        }

        return;
    }

    // Update cur-slot/stable client raw statistics for bandwidth usage

    void ClientStatisticsTracker::updateBandwidthUsage(const uint32_t& local_client_worker_idx, const BandwidthUsage& bandwidth_usage, const bool& is_stresstest_phase)
    {
        checkPointers_();

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update cur-slot client raw statistics
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        tmp_curslot_client_raw_statistics_ptr->updateBandwidthUsage_(local_client_worker_idx, bandwidth_usage);

        perclientworker_curslot_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_curslot_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest_phase)
        {
            stable_client_raw_statistics_ptr_->updateBandwidthUsage_(local_client_worker_idx, bandwidth_usage);
        }

        return;
    }

    // (2) Switch cur-slot client raw statistics (invoked by client thread ClientWrapper)

    ClientAggregatedStatistics ClientStatisticsTracker::switchCurslotForClientRawStatistics(const uint32_t& target_slot_idx)
    {
        checkPointers_();

        // Switch cur-slot client raw statistics to update for the next time slot
        const uint32_t current_slot_idx = cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER);
        if (target_slot_idx != (current_slot_idx + 1))
        {
            // TODO: Add a boolean into SwitchSlotResponse to handle inconsistent slot idx issues, which may be caused by packet retransmission
            std::ostringstream oss;
            oss << "invalid target slot idx " << target_slot_idx << " for current slot idx " << current_slot_idx;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        cur_slot_idx_++;

        // Slot switch barrier to ensure that all client workers will realize the next time slot
        curslotSwitchBarrier_();

        // Aggregate client raw statistics of the current time slot
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(current_slot_idx);
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        ClientAggregatedStatistics tmp_client_aggregated_statistics(tmp_curslot_client_raw_statistics_ptr);

        // Clean for the current time slot to avoid the interferences of obselete statistics
        tmp_curslot_client_raw_statistics_ptr->clean();

        return tmp_client_aggregated_statistics;
    }

    // (3) Aggregate cur-slot/stable client raw statistics when benchmark is finished (invoked by main client thread ClientWrapper)

    uint32_t ClientStatisticsTracker::aggregateForFinishrun(ClientAggregatedStatistics& lastslot_client_aggregated_statistics, ClientAggregatedStatistics& stable_client_aggregated_statistics)
    {
        checkPointers_();

        // Aggregate cur-slot client raw statistics as per-slot client aggregated statistics for the last slot
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        lastslot_client_aggregated_statistics = ClientAggregatedStatistics(tmp_curslot_client_raw_statistics_ptr);

        // Aggregate stable client raw statistics into stable client aggregated statistics
        stable_client_aggregated_statistics = ClientAggregatedStatistics(stable_client_raw_statistics_ptr_);

        return cur_slot_idx_;
    }

    // For cur-slot client raw statistics

    ClientRawStatistics* ClientStatisticsTracker::getCurslotClientRawStatisticsPtr_(const uint32_t& slot_idx) const
    {
        assert(curslot_client_raw_statistics_ptr_list_.size() > 0);
        uint32_t curslot_client_raw_statistics_idx = slot_idx % curslot_client_raw_statistics_ptr_list_.size();
        assert(curslot_client_raw_statistics_idx >= 0 && curslot_client_raw_statistics_idx < curslot_client_raw_statistics_ptr_list_.size());

        ClientRawStatistics* tmp_client_raw_statistics_ptr = curslot_client_raw_statistics_ptr_list_[curslot_client_raw_statistics_idx];
        assert(tmp_client_raw_statistics_ptr != NULL);
        return tmp_client_raw_statistics_ptr;
    }

    void ClientStatisticsTracker::curslotSwitchBarrier_() const
    {
        assert(perclientworker_curslot_update_statuses_ != NULL);

        uint64_t prev_perclientworker_curslot_update_statuses[perclient_workercnt_];
        memset(prev_perclientworker_curslot_update_statuses, 0, perclient_workercnt_ * sizeof(uint64_t));

        // Get previous cur-slot update statuses
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            prev_perclientworker_curslot_update_statuses[i] = perclientworker_curslot_update_statuses_[i].load(Util::LOAD_CONCURRENCY_ORDER);
        }

        // Wait all client workers to realize the next time slot
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            while (true)
            {
                if (prev_perclientworker_curslot_update_statuses[i] < perclientworker_curslot_update_statuses_[i].load(Util::LOAD_CONCURRENCY_ORDER)) // Client worker has updated cur-slot raw statistics for at least one response after switching time slot
                {
                    break;
                }
                else if (perclientworker_curslot_update_flags_[i].load(Util::LOAD_CONCURRENCY_ORDER) == false) // Client worker is NOT updating cur-slot raw statistics
                {
                    break;
                }
            }
        }
        
        return;
    }

    // Other utility functions

    void ClientStatisticsTracker::checkPointers_() const
    {
        assert(perclientworker_curslot_update_statuses_ != NULL);

        assert(curslot_client_raw_statistics_ptr_list_.size() > 0);
        for (uint32_t i = 0; i < curslot_client_raw_statistics_ptr_list_.size(); i++)
        {
            assert(curslot_client_raw_statistics_ptr_list_[i] != NULL);
        }

        assert(stable_client_raw_statistics_ptr_ != NULL);
 
        return;
    }
}