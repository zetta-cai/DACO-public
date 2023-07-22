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
    const std::string ClientStatisticsTracker::kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx) : allow_update_(true)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        perclient_workercnt_ = perclient_workercnt;
        latency_histogram_size_ = Config::getLatencyHistogramSize();

        // (B) Non-const individual variables (latency_histogram_ is shared)

        // (B.1) For updated intermediate statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        cur_perclientworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        Util::initializeAtomicArray(cur_perclientworker_local_hitcnts_, perclient_workercnt, 0);

        perclientworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        Util::initializeAtomicArray(perclientworker_cooperative_hitcnts_, perclient_workercnt, 0);

        perclientworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        Util::initializeAtomicArray(perclientworker_reqcnts_, perclient_workercnt, 0);

        latency_histogram_ = new std::atomic<uint32_t>[latency_histogram_size_];
        assert(latency_histogram_ != NULL);
        for (uint32_t i = 0; i < latency_histogram_size_; i++)
        {
            latency_histogram_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        perclientworker_readcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_readcnts_ != NULL);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            perclientworker_readcnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        perclientworker_writecnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_writecnts_ != NULL);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            perclientworker_writecnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }
    }

    ClientStatisticsTracker::ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx) : allow_update_(false)
    {
        // (A) Const shared variables

        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();
        
        perclient_workercnt_ = 0;
        latency_histogram_size_ = 0;

        // (B) Non-const individual variables (latency_histogram_ is shared)

        // (B.1) For updated intermediate statistics

        cur_slot_idx_.store(0, Util::STORE_CONCURRENCY_ORDER);

        perclientworker_local_hitcnts_ = NULL;
        perclientworker_cooperative_hitcnts_ = NULL;
        perclientworker_reqcnts_ = NULL;

        latency_histogram_ = NULL;

        perclientworker_readcnts_ = NULL;
        perclientworker_writecnts_ = NULL;

        load_(filepath); // Load per-client statistics from binary file
    }

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        assert(perclientworker_local_hitcnts_ != NULL);
        delete[] perclientworker_local_hitcnts_;
        perclientworker_local_hitcnts_ = NULL;

        assert(perclientworker_cooperative_hitcnts_ != NULL);
        delete[] perclientworker_cooperative_hitcnts_;
        perclientworker_cooperative_hitcnts_ = NULL;
        
        assert(perclientworker_reqcnts_ != NULL);
        delete[] perclientworker_reqcnts_;
        perclientworker_reqcnts_ = NULL;

        assert(latency_histogram_ != NULL);
        delete[] latency_histogram_;
        latency_histogram_ = NULL;

        assert(perclientworker_readcnts_ != NULL);
        delete[] perclientworker_readcnts_;
        perclientworker_readcnts_ = NULL;

        assert(perclientworker_writecnts_ != NULL);
        delete[] perclientworker_writecnts_;
        perclientworker_writecnts_ = NULL;
    }

    std::atomic<uint32_t>* ClientStatisticsTracker::getPerclientworkerReqcnts() const
    {
        return perclientworker_reqcnts_;
    }
    
    std::atomic<uint32_t>* ClientStatisticsTracker::getLatencyHistogram() const
    {
        return latency_histogram_;
    }

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_local_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_local_hitcnts_[local_client_worker_idx]++;
        updateReqcnt(local_client_worker_idx);
        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_cooperative_hitcnts_[local_client_worker_idx]++;
        updateReqcnt(local_client_worker_idx);
        return;
    }

    void ClientStatisticsTracker::updateReqcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_reqcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_reqcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientStatisticsTracker::updateLatency(const uint32_t& latency_us)
    {
        assert(latency_histogram_ != NULL);

        if (latency_us < latency_histogram_size_)
        {
            latency_histogram_[latency_us]++;
        }
        else
        {
            latency_histogram_[latency_histogram_size_ - 1]++;
        }
        return;
    }

    void ClientStatisticsTracker::updateReadcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_readcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_readcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientStatisticsTracker::updateWritecnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_writecnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_writecnts_[local_client_worker_idx]++;
        return;
    }

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

    // Get per-client statistics for aggregation

    std::atomic<uint32_t>* ClientStatisticsTracker::getPerclientworkerLocalHitcnts() const
    {
        return perclientworker_local_hitcnts_;
    }
    
    std::atomic<uint32_t>* ClientStatisticsTracker::getPerclientworkerCooperativeHitcnts() const
    {
        return perclientworker_cooperative_hitcnts_;
    }

    uint32_t ClientStatisticsTracker::getPerclientWorkercnt() const
    {
        return perclient_workercnt_;
    }
    
    uint32_t ClientStatisticsTracker::getLatencyHistogramSize() const
    {
        return latency_histogram_size_;
    }

    std::atomic<uint32_t>* ClientStatisticsTracker::getPerclientworkerReadcnts() const
    {
        return perclientworker_readcnts_;
    }
    
    std::atomic<uint32_t>* ClientStatisticsTracker::getPerclientworkerWritecnts() const
    {
        return perclientworker_writecnts_;
    }
}