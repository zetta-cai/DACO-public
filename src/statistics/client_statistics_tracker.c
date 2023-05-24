#include "statistics/client_statistics_tracker.h"

#include <cstring> // memcpy memset
#include <random> // std::mt19937_64

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt) : perclient_workercnt_(perclient_workercnt)
    {
        latency_histogram_ = new std::atomic<uint32_t>[Config::getLatencyHistogramSize()];
        assert(latency_histogram_ != NULL);
        for (uint32_t i = 0; i < Config::getLatencyHistogramSize(); i++)
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

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        // Free latency histogram
        assert(latency_histogram_ != NULL);
        delete latency_histogram_;
        latency_histogram_ = NULL;

        // Free local hit counts
        assert(perworker_local_hitcnts_ != NULL);
        delete perworker_local_hitcnts_;
        perworker_local_hitcnts_ = NULL;

        // Free global hit counts
        assert(perworker_cooperative_hitcnts_ != NULL);
        delete perworker_cooperative_hitcnts_;
        perworker_cooperative_hitcnts_ = NULL;

        // Free request counts
        assert(perworker_reqcnts_ != NULL);
        delete perworker_reqcnts_;
        perworker_reqcnts_ = NULL;
    }

    void ClientStatisticsTracker::updateLocalHitcnt(const uint32_t& local_worker_index))
    {
        assert(perworker_local_hitcnts_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        perworker_local_hitcnts_[local_worker_idx_]++;
        updateReqcnt(local_worker_index);
        return;
    }

    void ClientStatisticsTracker::updateCooperativeHitcnt(const uint32_t& local_worker_index))
    {
        assert(perworker_cooperative_hitcnts_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        perworker_cooperative_hitcnts_[local_worker_idx_]++;
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

    void ClientStatisticsTracker::updateLatency(const uint32_t& local_worker_index), const uint32_t& latency_us)
    {
        assert(latency_histogram_ != NULL);
        assert(local_worker_index < perclient_workercnt_);

        if (latency_us < Config::getLatencyHistogramSize())
        {
            latency_histogram_[latency_us]++;
        }
        else
        {
            latency_histogram_[Config::getLatencyHistogramSize() - 1]++;
        }
        return;
    }

    void ClientStatisticsTracker::dump(const std::string& filepath)
    {
        std::string tmp_filepath = filepath;

        bool is_exist = Util::isFileExist(tmp_filepath);
        if (is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "statistics file " << tmp_filepath << " already exists!";
            //Util::dumpErrorMsg(kClassName, oss.str());
            //exit(1);
            Util::dumpWarnMsg(kClassNamem, oss.str());

            // Generate a random number as a random seed
            uint32_t random_seed = Util::getTimeBasedRandomSeed();
            std::mt19937_64 randgen(random_seed);
            std::uniform_int_distribution<uint32_t> uniform_dist; // Range from 0 to max uint32_t
            uint32_t random_number = uniform_dist(random_seed);

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

        // TODO: create file
        // TOOD: open file
        // TODO: write file
        // TODO: close file
    }
    
    void ClientStatisticsTracker::load(const std::string& filepath);
}