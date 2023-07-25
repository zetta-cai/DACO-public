#include "statistics/client_statistics_tracker.h"

#include <assert.h>
#include <fstream>
#include <random> // std::mt19937_64
#include <sstream>

#include "common/config.h"
#include "common/dynamic_array.h"
#include "common/util.h"

namespace covered
{
    const uint32_t ClientStatisticsTracker::CURSLOT_CLIENT_RAW_STATISTICS_CNT = 2;

    const std::string ClientStatisticsTracker::kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx, const uint32_t& curslot_client_raw_statistics_cnt) : allow_update_(true), perclient_workercnt_(perclient_workercnt)
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

        // (C) For per-slot client aggregated statistics

        perslot_client_aggregated_statistics_list_.resize(0);
    }

    ClientStatisticsTracker::ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx) : allow_update_(false), perclient_workercnt_(0)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // (B) Non-const individual variables (latency_histogram_ is shared)

        // (B.1) For cur-slot client raw statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        perclientworker_curslot_update_flags_ = NULL;
        perclientworker_curslot_update_statuses_ = NULL;
        curslot_client_raw_statistics_ptr_list_.resize(0);

        // (B.2) For stable client raw statistics

        stable_client_raw_statistics_ptr_ = NULL;

        // (C) For per-slot client aggregated statistics

        perslot_client_aggregated_statistics_list_.resize(0);

        load_(filepath); // Load client aggregated statistics from binary file
    }

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        if (allow_update_)
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
    }

    // (1) Update cur-slot/stable client raw statistics (invoked by client workers)

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
    {
        checkPointers_();
        assert(allow_update_ == true);

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
        assert(allow_update_ == true);

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
        assert(allow_update_ == true);

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
        assert(allow_update_ == true);

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
        assert(allow_update_ == true);

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
        assert(allow_update_ == true);

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

    void ClientStatisticsTracker::switchCurslotForClientRawStatistics()
    {
        checkPointers_();
        assert(allow_update_ == true);

        // Switch cur-slot client raw statistics to update for the next time slot
        const uint32_t current_slot_idx = cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER);
        cur_slot_idx_++;

        // Slot switch barrier to ensure that all client workers will realize the next time slot
        curslotSwitchBarrier_();

        // Aggregate client raw statistics of the current time slot
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(current_slot_idx);
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        ClientAggregatedStatistics tmp_client_aggregated_statistics(tmp_curslot_client_raw_statistics_ptr);

        // Update per-slot client aggregated statistics
        perslot_client_aggregated_statistics_list_.push_back(tmp_client_aggregated_statistics);

        // Clean for the current time slot to avoid the interferences of obselete statistics
        tmp_curslot_client_raw_statistics_ptr->clean();

        return;
    }

    bool ClientStatisticsTracker::isPerSlotAggregatedStatisticsStable(double& cache_hit_ratio)
    {
        checkPointers_();
        assert(allow_update_ == true);

        bool is_stable = false;

        const uint32_t slotcnt = perslot_client_aggregated_statistics_list_.size();

        if (slotcnt > 1)
        {
            //return true; // TMPDEBUG

            double cur_total_hit_ratio = perslot_client_aggregated_statistics_list_[slotcnt - 1].getTotalHitRatio();
            double prev_total_hit_ratio = perslot_client_aggregated_statistics_list_[slotcnt - 2].getTotalHitRatio();

            // TODO: we may treat cache as stable after cache is full
            if (cur_total_hit_ratio > 0.0d && prev_total_hit_ratio > 0.0d && cur_total_hit_ratio <= prev_total_hit_ratio)
            {
                is_stable = true;
                cache_hit_ratio = cur_total_hit_ratio;
            }
        }

        return is_stable;
    }

    // (3) Aggregate cur-slot/stable client raw statistics, and dump per-slot/stable client aggregated statistics for TotalStatisticsTracker (invoked by main client thread ClientWrapper)

    uint32_t ClientStatisticsTracker::aggregateAndDump(const std::string& filepath)
    {
        checkPointers_();
        assert(allow_update_ == true);

        // Aggregate cur-slot client raw statistics as per-slot client aggregated statistics for the last slot
        ClientRawStatistics* tmp_curslot_client_raw_statistics_ptr = getCurslotClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(tmp_curslot_client_raw_statistics_ptr != NULL);
        ClientAggregatedStatistics tmp_client_aggregated_statistics(tmp_curslot_client_raw_statistics_ptr);
        perslot_client_aggregated_statistics_list_.push_back(tmp_client_aggregated_statistics);

        // Aggregate stable client raw statistics into stable client aggregated statistics
        stable_client_aggregated_statistics_ = ClientAggregatedStatistics(stable_client_raw_statistics_ptr_);

        // Check dump filepath
        std::string tmp_filepath = checkFilepathForDump_(filepath);

        // Create and open a binary file for per-client statistics
        // NOTE: each client opens a unique file (no confliction among different clients)
        std::ostringstream oss;
        oss << "open file " << tmp_filepath << " for client statistics";
        Util::dumpNormalMsg(instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump per-slot/stable client aggregated statistics for TotalStatisticsTracker
        // Format: slotcnt + perslot_client_aggregated_statistics_list_ + stable_client_aggregated_statistics_
        uint32_t size = 0;
        // (1) slotcnt
        const uint32_t slotcnt = perslot_client_aggregated_statistics_list_.size();
        fs_ptr->write((const char*)&slotcnt, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (2) perslot_client_aggregated_statistics_list_
        for (uint32_t i = 0; i < slotcnt; i++)
        {
            DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
            uint32_t tmp_serialize_size = perslot_client_aggregated_statistics_list_[i].serialize(tmp_dynamic_array, 0);
            tmp_dynamic_array.writeBinaryFile(0, fs_ptr, tmp_serialize_size);
            size += tmp_serialize_size;
        }
        // (3) stable_client_aggregated_statistics_
        DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        uint32_t tmp_serialize_size = stable_client_aggregated_statistics_.serialize(tmp_dynamic_array, 0);
        tmp_dynamic_array.writeBinaryFile(0, fs_ptr, tmp_serialize_size);
        size += tmp_serialize_size;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (4) Get client aggregated statistics

    std::vector<ClientAggregatedStatistics> ClientStatisticsTracker::getPerslotClientAggregatedStatistics() const
    {
        return perslot_client_aggregated_statistics_list_;
    }

    ClientAggregatedStatistics ClientStatisticsTracker::getStableClientAggregatedStatistics() const
    {
        return stable_client_aggregated_statistics_;
    }

    // Used by aggregateAndDump() to check filepath

    std::string ClientStatisticsTracker::checkFilepathForDump_(const std::string& filepath) const
    {
        std::string tmp_filepath = filepath;

        // Parent directory must exit
        std::string parentDirpath = Util::getParentDirpath(filepath);
        assert(Util::isDirectoryExist(parentDirpath));

        // Check statistics file existence
        bool is_exist = Util::isFileExist(filepath, true);
        if (is_exist)
        {
            // File already exists
            std::ostringstream oss;
            oss << "statistics file " << filepath << " already exists!";
            //Util::dumpErrorMsg(instance_name_, oss.str());
            //exit(1);
            Util::dumpWarnMsg(instance_name_, oss.str());

            // Generate a random number as a random seed
            uint32_t random_seed = Util::getTimeBasedRandomSeed();
            std::mt19937_64 randgen(random_seed);
            std::uniform_int_distribution<uint32_t> uniform_dist; // Range from 0 to max uint32_t
            uint32_t random_number = uniform_dist(randgen);

            // Replace with a random filepath
            oss.clear(); // Clear error states
            oss.str(""); // Set content as empty string and reset read/write position as zero
            oss << filepath << "." << random_number;
            tmp_filepath = oss.str();

            // Dump hints
            oss.clear(); // Clear error states
            oss.str(""); // Set content as empty string and reset read/write position as zero
            oss << "use a random file path " << tmp_filepath << " for statistics!";
            Util::dumpNormalMsg(instance_name_, oss.str());
        }

        return tmp_filepath;
    }

    // Load per-slot/stable client aggregated statistics for TotalStatisticsTracker

    uint32_t ClientStatisticsTracker::load_(const std::string& filepath)
    {
        bool is_exist = Util::isFileExist(filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "statistics file " << filepath << " does not exist!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Open the existing binary file for per-client statistics
        std::fstream* fs_ptr = Util::openFile(filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Read per-slot/stable client aggregated statistics from file
        // Format: slotcnt + perslot_client_aggregated_statistics_list_ + stable_client_aggregated_statistics_
        uint32_t size = 0;
        // (1) slotcnt
        uint32_t slotcnt = 0;
        fs_ptr->read((char *)&slotcnt, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (2) perslot_client_aggregated_statistics_list_
        perslot_client_aggregated_statistics_list_.resize(slotcnt);
        for (uint32_t i = 0; i < slotcnt; i++)
        {
            DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
            tmp_dynamic_array.readBinaryFile(0, fs_ptr, ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
            uint32_t tmp_deserialize_size = perslot_client_aggregated_statistics_list_[i].deserialize(tmp_dynamic_array, 0);
            size += tmp_deserialize_size;
        }
        // (3) stable_client_aggregated_statistics_
        DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        tmp_dynamic_array.readBinaryFile(0, fs_ptr, ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        uint32_t tmp_deserialize_size = stable_client_aggregated_statistics_.deserialize(tmp_dynamic_array, 0);
        size += tmp_deserialize_size;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // For cur-slot client raw statistics

    ClientRawStatistics* ClientStatisticsTracker::getCurslotClientRawStatisticsPtr_(const uint32_t& slot_idx) const
    {
        if (allow_update_)
        {
            assert(curslot_client_raw_statistics_ptr_list_.size() > 0);
            uint32_t curslot_client_raw_statistics_idx = slot_idx % curslot_client_raw_statistics_ptr_list_.size();
            assert(curslot_client_raw_statistics_idx >= 0 && curslot_client_raw_statistics_idx < curslot_client_raw_statistics_ptr_list_.size());

            return curslot_client_raw_statistics_ptr_list_[curslot_client_raw_statistics_idx];
        }
        else
        {
            return NULL;
        }
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
        if (allow_update_)
        {
            assert(perclientworker_curslot_update_statuses_ != NULL);

            assert(curslot_client_raw_statistics_ptr_list_.size() > 0);
            for (uint32_t i = 0; i < curslot_client_raw_statistics_ptr_list_.size(); i++)
            {
                assert(curslot_client_raw_statistics_ptr_list_[i] != NULL);
            }

            assert(stable_client_raw_statistics_ptr_ != NULL);
        }
        return;
    }
}