#include "statistics/client_statistics_tracker.h"

#include <assert.h>
#include <fstream>
#include <random> // std::mt19937_64
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"

namespace covered
{
    const std::string kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt, uint32_t latency_histogram_size)
    {
        perclient_workercnt_ = perclient_workercnt;
        latency_histogram_size_ = latency_histogram_size;

        latency_histogram_ = new std::atomic<uint32_t>[latency_histogram_size_];
        assert(latency_histogram_ != NULL);
        for (uint32_t i = 0; i < latency_histogram_size_; i++)
        {
            latency_histogram_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        perworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perworker_local_hitcnts_ != NULL);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            perworker_local_hitcnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        perworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perworker_cooperative_hitcnts_ != NULL);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            perworker_cooperative_hitcnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        perworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt];;
        assert(perworker_reqcnts_ != NULL);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            perworker_reqcnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }
    }

    ClientStatisticsTracker::ClientStatisticsTracker(const std::string& filepath)
    {
        perclient_workercnt_ = 0;
        perworker_local_hitcnts_ = NULL;
        perworker_cooperative_hitcnts_ = NULL;
        perworker_reqcnts_ = NULL;

        latency_histogram_size_ = 0;
        latency_histogram_ = NULL;

        load_(filepath); // Load per-client statistics from binary file
    }

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        // Release latency histogram
        assert(latency_histogram_ != NULL);
        delete latency_histogram_;
        latency_histogram_ = NULL;

        // Release local hit counts
        assert(perworker_local_hitcnts_ != NULL);
        delete perworker_local_hitcnts_;
        perworker_local_hitcnts_ = NULL;

        // Release global hit counts
        assert(perworker_cooperative_hitcnts_ != NULL);
        delete perworker_cooperative_hitcnts_;
        perworker_cooperative_hitcnts_ = NULL;

        // Release request counts
        assert(perworker_reqcnts_ != NULL);
        delete perworker_reqcnts_;
        perworker_reqcnts_ = NULL;
    }

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_worker_index)
    {
        assert(perworker_local_hitcnts_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        perworker_local_hitcnts_[local_worker_index]++;
        updateReqcnt(local_worker_index);
        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_worker_index)
    {
        assert(perworker_cooperative_hitcnts_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        perworker_cooperative_hitcnts_[local_worker_index]++;
        updateReqcnt(local_worker_index);
        return;
    }

    void ClientStatisticsTracker::updateReqcnt(const uint32_t& local_worker_index)
    {
        assert(perworker_reqcnts_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        perworker_reqcnts_[local_worker_index]++;
        return;
    }

    void ClientStatisticsTracker::updateLatency(const uint32_t& local_worker_index, const uint32_t& latency_us)
    {
        assert(latency_histogram_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

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

    // Dump per-client statistics for TotalStatisticsTracker
    uint32_t ClientStatisticsTracker::dump(const std::string& filepath) const
    {
        std::string tmp_filepath = filepath;

        bool is_exist = Util::isFileExist(tmp_filepath);
        if (is_exist)
        {
            // File already exists
            std::ostringstream oss;
            oss << "statistics file " << tmp_filepath << " already exists!";
            //Util::dumpErrorMsg(kClassName, oss.str());
            //exit(1);
            Util::dumpWarnMsg(kClassName, oss.str());

            // Generate a random number as a random seed
            uint32_t random_seed = Util::getTimeBasedRandomSeed();
            std::mt19937_64 randgen(random_seed);
            std::uniform_int_distribution<uint32_t> uniform_dist; // Range from 0 to max uint32_t
            uint32_t random_number = uniform_dist(randgen);

            // Replace with a random filepath
            oss.clear(); // Clear error states
            oss.str(""); // Set content as empty string and reset read/write position as zero
            oss << tmp_filepath << "." << random_number;
            tmp_filepath = oss.str();

            // Dump hints
            oss.clear(); // Clear error states
            oss.str(""); // Set content as empty string and reset read/write position as zero
            oss << "use a random file path " << tmp_filepath << " for statistics!";
            Util::dumpDebugMsg(kClassName, oss.str());
        }

        // Create and open a binary file for per-client statistics
        std::fstream* fs_ptr = Util::openFile(tmp_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Write per-client statistics into file
        // Format: perclient_workercnt_ + perworker_local_hitcnts_ + perworker_cooperative_hitcnts_ + perworker_reqcnts_ + latency histogram size + latency_histogram_
        uint32_t size = 0;
        // (1) perclient_workercnt_
        uint32_t perclient_workercnt_bytes = dumpPerclientWorkercnt_(fs_ptr);
        size += perclient_workercnt_bytes;
        // (2) perworker_local_hitcnts_
        uint32_t perworker_local_hitcnts_bytes = dumpPerworkerLocalHitcnts_(fs_ptr);
        size += perworker_local_hitcnts_bytes;
        // (3) perworker_cooperative_hitcnts_
        uint32_t perworker_cooperative_hitcnts_bytes = dumpPerworkerCooperativeHitcnts_(fs_ptr);
        size += perworker_cooperative_hitcnts_bytes;
        // (4) perworker_reqcnts_
        uint32_t perworker_reqcnts_bytes = dumpPerworkerReqcnts_(fs_ptr);
        size += perworker_reqcnts_bytes;
        // (5) latency histogram size
        uint32_t latency_histogram_size_bytes = dumpLatencyHistogramSize_(fs_ptr);
        size += latency_histogram_size_bytes;
        // (6) latency_histogram_
        uint32_t latency_histogram_bytes = dumpLatencyHistogram_(fs_ptr);
        size += latency_histogram_bytes;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size;
    }

    // Used by dump() to dump per-client statistics

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

    uint32_t ClientStatisticsTracker::dumpPerworkerLocalHitcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perworker_local_hitcnts_ != NULL);

        uint32_t perworker_local_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_local_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_local_hitcnt = perworker_local_hitcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_local_hitcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perworker_local_hitcnts_bytes);
        return perworker_local_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerworkerCooperativeHitcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perworker_cooperative_hitcnts_ != NULL);

        uint32_t perworker_cooperative_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_cooperative_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_cooperative_hitcnt = perworker_cooperative_hitcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_cooperative_hitcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perworker_cooperative_hitcnts_bytes);
        return perworker_cooperative_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::dumpPerworkerReqcnts_(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        assert(perworker_reqcnts_ != NULL);

        uint32_t perworker_reqcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_reqcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_reqcnt = perworker_reqcnts_[i].load(Util::LOAD_CONCURRENCY_ORDER);
            dynamic_array.deserialize(i * sizeof(uint32_t),(const char*)&tmp_reqcnt, sizeof(uint32_t));
        }
        dynamic_array.writeBinaryFile(0, fs_ptr, perworker_reqcnts_bytes);
        return perworker_reqcnts_bytes;
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

    // Load per-client statistics for TotalStatisticsTracker
    uint32_t ClientStatisticsTracker::load_(const std::string& filepath)
    {
        bool is_exist = Util::isFileExist(filepath);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "statistics file " << filepath << " does not exist!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Open the existing binary file for per-client statistics
        std::fstream* fs_ptr = Util::openFile(filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Read per-client statistics from file
        // Format: perclient_workercnt_ + perworker_local_hitcnts_ + perworker_cooperative_hitcnts_ + perworker_reqcnts_ + latency histogram size + latency_histogram_
        uint32_t size = 0;
        // (1) perclient_workercnt_
        uint32_t perclient_workercnt_bytes = loadPerclientWorkercnt_(fs_ptr);
        size += perclient_workercnt_bytes;
        // (2) perworker_local_hitcnts_
        uint32_t perworker_local_hitcnts_bytes = loadPerworkerLocalHitcnts_(fs_ptr);
        size += perworker_local_hitcnts_bytes;
        // (3) perworker_cooperative_hitcnts_
        uint32_t perworker_cooperative_hitcnts_bytes = loadPerworkerCooperativeHitcnts_(fs_ptr);
        size += perworker_cooperative_hitcnts_bytes;
        // (4) perworker_reqcnts_
        uint32_t perworker_reqcnts_bytes = loadPerworkerReqcnts_(fs_ptr);
        size += perworker_reqcnts_bytes;
        // (5) latency histogram size
        uint32_t latency_histogram_size_bytes = loadLatencyHistogramSize_(fs_ptr);
        size += latency_histogram_size_bytes;
        // (6) latency_histogram_
        uint32_t latency_histogram_bytes = loadLatencyHistogram_(fs_ptr);
        size += latency_histogram_bytes;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size;
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

    uint32_t ClientStatisticsTracker::loadPerworkerLocalHitcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perworker_local_hitcnts_ == NULL && perclient_workercnt_ > 0);
        perworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perworker_local_hitcnts_ != NULL);

        uint32_t perworker_local_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_local_hitcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perworker_local_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_local_hitcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_local_hitcnt, sizeof(uint32_t));
            perworker_local_hitcnts_[i].store(tmp_local_hitcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perworker_local_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerworkerCooperativeHitcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perworker_cooperative_hitcnts_ == NULL && perclient_workercnt_ > 0);
        perworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perworker_cooperative_hitcnts_ != NULL);

        uint32_t perworker_cooperative_hitcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_cooperative_hitcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perworker_cooperative_hitcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_cooperative_hitcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_cooperative_hitcnt, sizeof(uint32_t));
            perworker_cooperative_hitcnts_[i].store(tmp_cooperative_hitcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perworker_cooperative_hitcnts_bytes;
    }

    uint32_t ClientStatisticsTracker::loadPerworkerReqcnts_(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);
        assert(perworker_reqcnts_ == NULL && perclient_workercnt_ > 0);
        perworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt_];
        assert(perworker_reqcnts_ != NULL);

        uint32_t perworker_reqcnts_bytes = perclient_workercnt_ * sizeof(uint32_t);
        DynamicArray dynamic_array(perworker_reqcnts_bytes);
        dynamic_array.readBinaryFile(0, fs_ptr, perworker_reqcnts_bytes);
        for (uint32_t i = 0; i < perclient_workercnt_; i++)
        {
            uint32_t tmp_reqcnt = 0;
            dynamic_array.serialize(i * sizeof(uint32_t),(char*)&tmp_reqcnt, sizeof(uint32_t));
            perworker_reqcnts_[i].store(tmp_reqcnt, Util::STORE_CONCURRENCY_ORDER);
        }
        return perworker_reqcnts_bytes;
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

    // Get per-client statistics for aggregation

    std::atomic<uint32_t>* ClientStatisticsTracker::getPerworkerLocalHitcnts() const
    {
        return perworker_local_hitcnts_;
    }
    
    std::atomic<uint32_t>* ClientStatisticsTracker::getPerworkerCooperativeHitcnts() const
    {
        return perworker_reqcnts_;
    }

    std::atomic<uint32_t>* ClientStatisticsTracker::getPerworkerReqcnts() const
    {
        return perworker_reqcnts_;
    }
    
    std::atomic<uint32_t>* ClientStatisticsTracker::getLatencyHistogram() const
    {
        return latency_histogram_;
    }

    uint32_t ClientStatisticsTracker::getPerclientWorkercnt() const
    {
        return perclient_workercnt_;
    }
    
    uint32_t ClientStatisticsTracker::getLatencyHistogramSize() const
    {
        return latency_histogram_size_;
    }
}