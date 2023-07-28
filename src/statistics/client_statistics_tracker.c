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
        Util::initializeAtomicArray(perclientworker_curslot_update_flags_, perclient_workercnt_, false);

        perclientworker_curslot_update_statuses_ = new std::atomic<uint64_t>[perclient_workercnt];
        assert(perclientworker_curslot_update_statuses_ != NULL);
        Util::initializeAtomicArray(perclientworker_curslot_update_statuses_, perclient_workercnt, 0);

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

    // (1) Update cur-slot/stable client raw statistics (invoked by client workers)

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateLocalHitcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateCooperativeHitcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateReqcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateReqcnt_(local_client_worker_idx);
        }
        
        return;
    }

    void ClientStatisticsTracker::updateLatency(const uint32_t& local_client_worker_idx, const uint32_t& latency_us, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateLatency_(latency_us);
        }
        
        return;
    }

    void ClientStatisticsTracker::updateReadcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateReadcnt_(local_client_worker_idx);
        }

        return;
    }

    void ClientStatisticsTracker::updateWritecnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
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
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateWritecnt_(local_client_worker_idx);
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

        return curslot_client_raw_statistics_ptr_list_[curslot_client_raw_statistics_idx];
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