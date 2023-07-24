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
    const uint32_t ClientStatisticsTracker::INTERMEDIATE_SLOT_CNT = 2;

    const std::string ClientStatisticsTracker::kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx, const uint32_t& intermediate_slot_cnt) : allow_update_(true), perclient_workercnt_(perclient_workercnt)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // (B) Non-const individual variables

        // (B.1) For intermediate client raw statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        perclientworker_intermediate_update_flags_ = new std::atomic<bool>[perclient_workercnt_];
        assert(perclientworker_intermediate_update_flags_ != NULL);
        Util::initializeAtomicArray(perclientworker_intermediate_update_flags_, perclient_workercnt_, false);

        perclientworker_intermediate_update_statuses_ = new std::atomic<uint64_t>[perclient_workercnt];
        assert(perclientworker_intermediate_update_statuses_ != NULL);
        Util::initializeAtomicArray(perclientworker_intermediate_update_statuses_, perclient_workercnt, 0);

        intermediate_client_raw_statistics_ptr_list_.resize(intermediate_slot_cnt);
        for (uint32_t i = 0; i < intermediate_client_raw_statistics_ptr_list_.size(); i++)
        {
            intermediate_client_raw_statistics_ptr_list_[i] = new ClientRawStatistics(perclient_workercnt);
            assert(intermediate_client_raw_statistics_ptr_list_[i] != NULL);
        }

        // (B.2) For stable client raw statistics

        stable_client_raw_statistics_ptr_ = new ClientRawStatistics(perclient_workercnt);
        assert(stable_client_raw_statistics_ptr_ != NULL);

        // (C) For per-slot client aggregated statistics

        perslot_client_aggregated_statistics_list_.resize(0);
    }

    ClientStatisticsTracker::ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx) : allow_update_(false), perclient_workercnt_(perclient_workercnt)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // (B) Non-const individual variables (latency_histogram_ is shared)

        // (B.1) For intermediate client raw statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        perclientworker_intermediate_update_flags_ = NULL;
        perclientworker_intermediate_update_statuses_ = NULL;
        intermediate_client_raw_statistics_ptr_list_.resize(0);

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
            assert(perclientworker_intermediate_update_flags_ != NULL);
            delete[] perclientworker_intermediate_update_flags_;
            perclientworker_intermediate_update_flags_ = NULL;

            assert(perclientworker_intermediate_update_statuses_ != NULL);
            delete[] perclientworker_intermediate_update_statuses_;
            perclientworker_intermediate_update_statuses_ = NULL;

            assert(intermediate_client_raw_statistics_ptr_list_.size() > 0);
            for (uint32_t i = 0; i < intermediate_client_raw_statistics_ptr_list_.size(); i++)
            {
                assert(intermediate_client_raw_statistics_ptr_list_[i] != NULL);
                delete intermediate_client_raw_statistics_ptr_list_[i];
                intermediate_client_raw_statistics_ptr_list_[i] = NULL;
            }

            assert(stable_client_raw_statistics_ptr_ != NULL);
            delete stable_client_raw_statistics_ptr_;
            stable_client_raw_statistics_ptr_ = NULL;
        }
    }

    // (1) Update intermediate/stable client raw statistics (invoked by client workers)

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateLocalHitcnt_();

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateLocalHitcnt_();
        }

        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_client_worker_idx)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateCooperativeHitcnt_();

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateCooperativeHitcnt_();
        }

        return;
    }

    void ClientStatisticsTracker::updateReqcnt(const uint32_t& local_client_worker_idx)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateReqcnt_();

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateReqcnt_();
        }
        
        return;
    }

    void ClientStatisticsTracker::updateLatency(const uint32_t& latency_us)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateLatency_();


        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateLatency_();
        }
        
        return;
    }

    void ClientStatisticsTracker::updateReadcnt(const uint32_t& local_client_worker_idx)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateReadcnt_();

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateReadcnt_();
        }

        return;
    }

    void ClientStatisticsTracker::updateWritecnt(const uint32_t& local_client_worker_idx)
    {
        checkPointers_();
        assert(allow_update == true);

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(true, Util::STORE_CONCURRENCY_ORDER);

        // Update intermediate client raw statistics
        ClientRawStatistics* cur_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER));
        assert(cur_intermediate_client_raw_statistics_ptr != NULL);
        cur_intermediate_client_raw_statistics_ptr->updateWritecnt_();

        perclientworker_intermediate_update_flags_[local_client_worker_idx].store(false, Util::STORE_CONCURRENCY_ORDER);
        perclientworker_intermediate_update_statuses_[local_client_worker_idx]++;

        // Update stable client raw statistics for stresstest phase
        if (is_stresstest)
        {
            stable_client_raw_statistics_ptr_->updateWritecnt_();
        }

        return;
    }

    // (2) Switch time slot for intermediate client raw statistics (invoked by client thread ClientWrapper)

    void ClientStatisticsTracker::switchIntermediateSlot()
    {
        checkPointers_();
        assert(allow_update == true);

        // Switch to update intermediate client raw statistics for the next time slot
        const uint32_t current_slot_idx = cur_slot_idx_.load(Util::LOAD_CONCURRENCY_ORDER);
        cur_slot_idx_++;

        // Slot switch barrier to ensure that all client workers will realize the next time slot
        intermediateSlotSwitchBarrier_();

        // Aggregate client raw statistics of the current time slot
        ClientRawStatistics* current_intermediate_client_raw_statistics_ptr = getIntermediateClientRawStatisticsPtr_(current_slot_idx);
        assert(current_intermediate_client_raw_statistics_ptr != NULL);
        ClientAggregatedStatistics tmp_client_aggregated_statistics(current_intermediate_client_raw_statistics_ptr);

        // Update per-slot client aggregated statistics
        perslot_client_aggregated_statistics_list_.push_back(tmp_client_aggregated_statistics);

        // Clean for the current time slot to avoid the interferences of obselete statistics
        current_intermediate_client_raw_statistics_ptr->clean();

        return;
    }

    // (3) Dump client per-slot/stable aggregated statistics (invoked by main client thread ClientWrapper)

    // Dump per-client statistics for TotalStatisticsTracker
    uint32_t ClientStatisticsTracker::dump(const std::string& filepath) const
    {
        std::string tmp_filepath = checkFilepathForDump_(filepath);

        // Create and open a binary file for per-client statistics
        // NOTE: each client opens a unique file (no confliction among different clients)
        std::ostringstream oss;
        oss << "open file " << tmp_filepath << " for client statistics";
        Util::dumpDebugMsg(instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Write per-client statistics into file
        // Format: perclient_workercnt_ + perclientworker_local_hitcnts_ + perclientworker_cooperative_hitcnts_ + perclientworker_reqcnts_ + latency histogram size + latency_histogram_
        uint32_t size = 0;
        // (1) perclient_workercnt_
        uint32_t perclient_workercnt_bytes = dumpPerclientWorkercnt_(fs_ptr);
        size += perclient_workercnt_bytes;
        // (2) perclientworker_local_hitcnts_
        uint32_t perclientworker_local_hitcnts_bytes = dumpPerclientworkerLocalHitcnts_(fs_ptr);
        size += perclientworker_local_hitcnts_bytes;
        // (3) perclientworker_cooperative_hitcnts_
        uint32_t perclientworker_cooperative_hitcnts_bytes = dumpPerclientworkerCooperativeHitcnts_(fs_ptr);
        size += perclientworker_cooperative_hitcnts_bytes;
        // (4) perclientworker_reqcnts_
        uint32_t perclientworker_reqcnts_bytes = dumpPerclientworkerReqcnts_(fs_ptr);
        size += perclientworker_reqcnts_bytes;
        // (5) latency histogram size
        uint32_t latency_histogram_size_bytes = dumpLatencyHistogramSize_(fs_ptr);
        size += latency_histogram_size_bytes;
        // (6) latency_histogram_
        uint32_t latency_histogram_bytes = dumpLatencyHistogram_(fs_ptr);
        size += latency_histogram_bytes;
        // (7) perclientworker_readcnts_
        uint32_t perclientworker_readcnts_bytes = dumpPerclientworkerReadcnts_(fs_ptr);
        size += perclientworker_readcnts_bytes;
        // (8) perclientworker_writecnts_
        uint32_t perclientworker_writecnts_bytes = dumpPerclientworkerWritecnts_(fs_ptr);
        size += perclientworker_writecnts_bytes;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // Used by dump() to check filepath and dump per-client statistics

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

    uint32_t ClientStatisticsTracker::dumpPerclientWorkercnt_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // No need for endianess conversion due to file I/O instead of network I/O
        uint32_t perclient_workercnt_bytes = sizeof(uint32_t);
        DynamicArray dynamic_array(perclient_workercnt_bytes);
        dynamic_array.deserialize(0, (const char*)&perclient_workercnt_, perclient_workercnt_bytes);
        dynamic_array.writeBinaryFile(0, fs_ptr, perclient_workercnt_bytes);
        return perclient_workercnt_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerclientworkerLocalHitcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_local_hitcnts_ != NULL);

        uint32_t perclientworker_local_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_local_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_local_hitcnt = perclientworker_local_hitcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_local_hitcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perclientworker_local_hitcnts_bytes);
        return perclientworker_local_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerclientworkerCooperativeHitcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_cooperative_hitcnts_ != NULL);

        uint32_t perclientworker_cooperative_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_cooperative_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_cooperative_hitcnt = perclientworker_cooperative_hitcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_cooperative_hitcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perclientworker_cooperative_hitcnts_bytes);
        return perclientworker_cooperative_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerclientworkerReqcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_reqcnts_ != NULL);

        uint32_t perclientworker_reqcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_reqcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_reqcnt = perclientworker_reqcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_reqcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perclientworker_reqcnts_bytes);
        return perclientworker_reqcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpLatencyHistogramSize_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // No need for endianess conversion due to file I/O instead of network I/O
        uint32_t latency_histogram_size_bytes = sizeof(uint32_t);
        DynamicArray dynamic_array(latency_histogram_size_bytes);
        dynamic_array.deserialize(0, (const char*)&latency_histogram_size_, latency_histogram_size_bytes);
        dynamic_array.writeBinaryFile(0, fs_ptr, latency_histogram_size_bytes);
        return latency_histogram_size_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpLatencyHistogram_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(latency_histogram_ != NULL);

        uint32_t latency_histogram_bytes = latency_histogram_size_ * sizeof(uint32_t);
        DynamicArray dynamic_array(latency_histogram_bytes);
        for (uint32_t i = 0; i < latency_histogram_size_; i++)
        {
            uint32_t tmp_latency_histogram_counter = latency_histogram_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_latency_histogram_counter, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, latency_histogram_bytes);
        return latency_histogram_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerclientworkerReadcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_readcnts_ != NULL);

        uint32_t perclientworker_readcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_readcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_readcnt = perclientworker_readcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t), (const char*)&tmp_readcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perclientworker_readcnts_bytes);
        return perclientworker_readcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerclientworkerWritecnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_writecnts_ != NULL);

        uint32_t perclientworker_writecnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_writecnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_writecnt = perclientworker_writecnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t), (const char*)&tmp_writecnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perclientworker_writecnts_bytes);
        return perclientworker_writecnts_bytes;
    }

    // Load per-client statistics for TotalStatisticsTracker
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

        // Read per-client statistics from file
        // Format: perclient_workercnt_ + perclientworker_local_hitcnts_ + perclientworker_cooperative_hitcnts_ + perclientworker_reqcnts_ + latency histogram size + latency_histogram_
        uint32_t size = 0;
        // (1) perclient_workercnt_
        uint32_t perclient_workercnt_bytes = loadPerclientWorkercnt_(fs_ptr);
        size += perclient_workercnt_bytes;
        // (2) perclientworker_local_hitcnts_
        uint32_t perclientworker_local_hitcnts_bytes = loadPerclientworkerLocalHitcnts_(fs_ptr);
        size += perclientworker_local_hitcnts_bytes;
        // (3) perclientworker_cooperative_hitcnts_
        uint32_t perclientworker_cooperative_hitcnts_bytes = loadPerclientworkerCooperativeHitcnts_(fs_ptr);
        size += perclientworker_cooperative_hitcnts_bytes;
        // (4) perclientworker_reqcnts_
        uint32_t perclientworker_reqcnts_bytes = loadPerclientworkerReqcnts_(fs_ptr);
        size += perclientworker_reqcnts_bytes;
        // (5) latency histogram size
        uint32_t latency_histogram_size_bytes = loadLatencyHistogramSize_(fs_ptr);
        size += latency_histogram_size_bytes;
        // (6) latency_histogram_
        uint32_t latency_histogram_bytes = loadLatencyHistogram_(fs_ptr);
        size += latency_histogram_bytes;
        // (7) perclientworker_readcnts_
        uint32_t perclientworker_readcnts_bytes = loadPerclientworkerReadcnts_(fs_ptr);
        size += perclientworker_readcnts_bytes;
        // (8) perclientworker_writecnts_
        uint32_t perclientworker_writecnts_bytes = loadPerclientworkerWritecnts_(fs_ptr);
        size += perclientworker_writecnts_bytes;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // Used by load_() to load per-client statistics

    uint32_t ClientStatisticsTracker::loadPerclientWorkercnt_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // No need for endianess conversion due to file I/O instead of network I/O
        uint32_t perclient_workercnt_bytes = sizeof(uint32_t);
        DynamicArray dynamic_array(perclient_workercnt_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclient_workercnt_bytes);
        dynamic_array.serialize(0, (char*)&perclient_workercnt_, perclient_workercnt_bytes);
        return perclient_workercnt_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerclientworkerLocalHitcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_local_hitcnts_ == NULL && perclient_workercnt_ > 0);
        perclientworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perclientworker_local_hitcnts_ != NULL);

        uint32_t perclientworker_local_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_local_hitcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclientworker_local_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_local_hitcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_local_hitcnt, sizeof(uint32_t));
            perclientworker_local_hitcnts_[i].store(tmp_local_hitcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perclientworker_local_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerclientworkerCooperativeHitcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_cooperative_hitcnts_ == NULL && perclient_workercnt_ > 0);
        perclientworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perclientworker_cooperative_hitcnts_ != NULL);

        uint32_t perclientworker_cooperative_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_cooperative_hitcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclientworker_cooperative_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_cooperative_hitcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_cooperative_hitcnt, sizeof(uint32_t));
            perclientworker_cooperative_hitcnts_[i].store(tmp_cooperative_hitcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perclientworker_cooperative_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerclientworkerReqcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_reqcnts_ == NULL && perclient_workercnt_ > 0);
        perclientworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perclientworker_reqcnts_ != NULL);

        uint32_t perclientworker_reqcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_reqcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclientworker_reqcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_reqcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_reqcnt, sizeof(uint32_t));
            perclientworker_reqcnts_[i].store(tmp_reqcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perclientworker_reqcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadLatencyHistogramSize_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // No need for endianess conversion due to file I/O instead of network I/O
        uint32_t latency_histogram_size_bytes = sizeof(uint32_t);
        DynamicArray dynamic_array(latency_histogram_size_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, latency_histogram_size_bytes);
        dynamic_array.serialize(0, (char*)&latency_histogram_size_, latency_histogram_size_bytes);
        return latency_histogram_size_bytes;
    }

    uint32_t ClientStatisticsTracker::loadLatencyHistogram_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(latency_histogram_ == NULL && latency_histogram_size_ > 0);
        latency_histogram_ = new std::atomic<uint32_t>[latency_histogram_size_];
        assert(latency_histogram_ != NULL);

        uint32_t latency_histogram_bytes = latency_histogram_size_ * sizeof(uint32_t);
        DynamicArray dynamic_array(latency_histogram_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, latency_histogram_bytes);
        for (uint32_t i = 0; i < latency_histogram_size_; i++)
        {
            uint32_t tmp_latency_histogram_counter = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_latency_histogram_counter, sizeof(uint32_t));
            latency_histogram_[i].store(tmp_latency_histogram_counter, Util::STORE_CONCURRENCY_ORDER);
        }
        return latency_histogram_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerclientworkerReadcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_readcnts_ == NULL && perclient_workercnt_ > 0);
        perclientworker_readcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perclientworker_readcnts_ != NULL);

        uint32_t perclientworker_readcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_readcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclientworker_readcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_readcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_readcnt, sizeof(uint32_t));
            perclientworker_readcnts_[i].store(tmp_readcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perclientworker_readcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerclientworkerWritecnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perclientworker_writecnts_ == NULL && perclient_workercnt_ > 0);
        perclientworker_writecnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perclientworker_writecnts_ != NULL);

        uint32_t perclientworker_writecnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perclientworker_writecnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perclientworker_writecnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_writecnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_writecnt, sizeof(uint32_t));
            perclientworker_writecnts_[i].store(tmp_writecnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perclientworker_writecnts_bytes;
    }

    // For intermediate client raw statistics

    ClientRawStatistics* ClientStatisticsTracker::getIntermediateClientRawStatisticsPtr_(const uint32_t& slot_idx)
    {
        if (allow_update_)
        {
            assert(intermediate_client_raw_statistics_ptr_list_.size() > 0);
            uint32_t intermediate_client_raw_statistics_idx = slot_idx % intermediate_client_raw_statistics_ptr_list_.size();
            assert(intermediate_client_raw_statistics_idx >= 0 && intermediate_client_raw_statistics_idx < intermediate_client_raw_statistics_ptr_list_.size());

            return intermediate_client_raw_statistics_ptr_list_[intermediate_client_raw_statistics_idx];
        }
        else
        {
            return NULL;
        }
    }

    void ClientStatisticsTracker::intermediateSlotSwitchBarrier_() const
    {
        assert(perclientworker_intermediate_update_statuses_ != NULL);

        uint64_t prev_perclientworker_intermediate_update_statuses[perclient_workercnt_];
        memset(prev_perclientworker_intermediate_update_statuses, 0, perclient_workercnt_ * sizeof(uint64_t));

        // Get previous intermediate update statuses
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            prev_perclientworker_intermediate_update_statuses[i] = perclientworker_intermediate_update_statuses_[i].load(Util::LOAD_CONCURRENCY_ORDER);
        }

        // Wait all client workers to realize the next time slot
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            while (true)
            {
                if (prev_perclientworker_intermediate_update_statuses[i] < perclientworker_intermediate_update_statuses_[i].load(Util::LOAD_CONCURRENCY_ORDER)) // Client worker has updated intermediate raw statistics for at least one response after switching time slot
                {
                    break;
                }
                else if (perclientworker_intermediate_update_flags_[i].load(Util::LOAD_CONCURRENCY_ORDER) == false) // Client worker is NOT updating intermediate raw statistics
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
            assert(perclientworker_intermediate_update_statuses_ != NULL);

            assert(intermediate_client_raw_statistics_ptr_list_.size() > 0);
            for (uint32_t i = 0; i < intermediate_client_raw_statistics_ptr_list_.size(); i++)
            {
                assert(intermediate_client_raw_statistics_ptr_list_[i] != NULL);
            }

            assert(stable_client_raw_statistics_ptr_ != NULL);
        }
        return;
    }
}