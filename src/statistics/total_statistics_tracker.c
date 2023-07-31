#include "statistics/total_statistics_tracker.h"

#include <assert.h>
//#include <cstring> // memset
#include <fstream>
#include <random> // std::mt19937_64
#include <sstream>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const double TotalStatisticsTracker::CACHE_UTILIZATION_THRESHOLD_FOR_FILLUP = 0.999; // >= 99.9% cache utilization
    const uint64_t TotalStatisticsTracker::CACHE_MARGIN_BYTES_IOTA_FOR_FILLUP = 1 * 1024 * 1024; // <= 1MiB
    const double TotalStatisticsTracker::CACHE_HIT_RATIO_CHANGE_THRESHOLD_FOR_STABLE = 0.001; // <= 0.1% cache hit ratio change

    const std::string TotalStatisticsTracker::kClassName("TotalStatisticsTracker");
    
    TotalStatisticsTracker::TotalStatisticsTracker() : allow_update_(true)
    {
        perslot_total_aggregated_statistics_.resize(0);
    }

    TotalStatisticsTracker::TotalStatisticsTracker(const std::string& filepath) : allow_update_(false)
    {
        perslot_total_aggregated_statistics_.resize(0);

        uint32_t load_size = load_(filepath); // Load per-slot/stable total aggregated statistics from binary file

        std::ostringstream oss;
        oss << "load total aggregated statistics of " << load_size << " bytes";
        Util::dumpNormalMsg(kClassName, oss.str());
    }

        
    TotalStatisticsTracker::~TotalStatisticsTracker() {}

    void TotalStatisticsTracker::updatePerslotTotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics)
    {
        assert(allow_update_ == true);

        const uint32_t clientcnt = curslot_perclient_aggregated_statistics.size();
        assert(clientcnt > 0);

        TotalAggregatedStatistics total_aggregated_statistics(curslot_perclient_aggregated_statistics);
        perslot_total_aggregated_statistics_.push_back(total_aggregated_statistics);

        return;
    }

    void TotalStatisticsTracker::updateStableTotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& stable_perclient_aggregated_statistics)
    {
        assert(allow_update_ == true);

        const uint32_t clientcnt = stable_perclient_aggregated_statistics.size();
        assert(clientcnt > 0);

        TotalAggregatedStatistics total_aggregated_statistics(stable_perclient_aggregated_statistics);
        stable_total_aggregated_statistics_ = total_aggregated_statistics;

        return;
    }

    uint64_t TotalStatisticsTracker::getCurslotTotalCacheMarginBytes() const
    {
        assert(allow_update_ == true);

        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        assert(slotcnt > 0);

        uint64_t cur_total_cache_margin_bytes = perslot_total_aggregated_statistics_[slotcnt - 1].getTotalCacheMarginBytes();

        return cur_total_cache_margin_bytes;
    }
    
    double TotalStatisticsTracker::getCurslotTotalCacheUtilization() const
    {
        assert(allow_update_ == true);

        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        assert(slotcnt > 0);

        double cur_total_cache_utilization = perslot_total_aggregated_statistics_[slotcnt - 1].getTotalCacheUtilization();

        return cur_total_cache_utilization;
    }

    double TotalStatisticsTracker::getCurslotTotalHitRatio() const
    {
        assert(allow_update_ == true);

        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        assert(slotcnt > 0);

        double cur_total_hit_ratio = perslot_total_aggregated_statistics_[slotcnt - 1].getTotalHitRatio();

        return cur_total_hit_ratio;
    }

    double TotalStatisticsTracker::getPrevslotTotalHitRatio() const
    {
        assert(allow_update_ == true);

        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        assert(slotcnt > 1);

        double prev_total_hit_ratio = perslot_total_aggregated_statistics_[slotcnt - 2].getTotalHitRatio();

        return prev_total_hit_ratio;
    }

    bool TotalStatisticsTracker::isPerSlotTotalAggregatedStatisticsStable(double& cache_hit_ratio)
    {
        assert(allow_update_ == true);

        bool is_stable = false;

        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        if (slotcnt > 1)
        {
            //return true; // TMPDEBUG

            uint64_t cur_total_cache_margin_bytes = getCurslotTotalCacheMarginBytes();
            double cur_total_cache_utilization = getCurslotTotalCacheUtilization();
            double cur_total_hit_ratio = getCurslotTotalHitRatio();
            double prev_total_hit_ratio = getPrevslotTotalHitRatio();

            // If cache is filled up
            bool is_cache_fillup = false;
            if (cur_total_cache_margin_bytes <= CACHE_MARGIN_BYTES_IOTA_FOR_FILLUP || cur_total_cache_utilization >= CACHE_UTILIZATION_THRESHOLD_FOR_FILLUP)
            {
                is_cache_fillup = true;
            }

            // If cache becomes stable
            bool is_cache_stable = false;
            if (is_cache_fillup)
            {
                double abs_cache_hit_ratio_change = (cur_total_hit_ratio >= prev_total_hit_ratio)?(cur_total_hit_ratio - prev_total_hit_ratio):(prev_total_hit_ratio - cur_total_hit_ratio);
                if (cur_total_hit_ratio > 0.0 && prev_total_hit_ratio > 0.0 && abs_cache_hit_ratio_change <= CACHE_HIT_RATIO_CHANGE_THRESHOLD_FOR_STABLE)
                {
                    is_cache_stable = true;
                }
            }

            if (is_cache_stable)
            {
                is_stable = true;
                cache_hit_ratio = cur_total_hit_ratio;
            }
        }

        return is_stable;
    }

    uint32_t TotalStatisticsTracker::dump(const std::string filepath) const
    {
        assert(allow_update_ == true);

        // Check dump filepath
        std::string tmp_filepath = checkFilepathForDump_(filepath);

        // Create and open a binary file for total aggregated statistics
        // NOTE: each client opens a unique file (no confliction among different clients)
        std::ostringstream oss;
        oss << "open file " << tmp_filepath << " for total aggregated statistics";
        Util::dumpNormalMsg(kClassName, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump per-slot/stable total aggregated statistics
        // Format: slotcnt + perslot_total_aggregated_statistics_ + stable_total_aggregated_statistics_
        uint32_t size = 0;
        // (1) slotcnt
        const uint32_t slotcnt = perslot_total_aggregated_statistics_.size();
        fs_ptr->write((const char*)&slotcnt, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (2) perslot_total_aggregated_statistics_
        for (uint32_t i = 0; i < slotcnt; i++)
        {
            DynamicArray tmp_dynamic_array(TotalAggregatedStatistics::getAggregatedStatisticsIOSize());
            uint32_t tmp_serialize_size = perslot_total_aggregated_statistics_[i].serialize(tmp_dynamic_array, 0);
            tmp_dynamic_array.writeBinaryFile(0, fs_ptr, tmp_serialize_size);
            size += tmp_serialize_size;
        }
        // (3) stable_total_aggregated_statistics_
        DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        uint32_t tmp_serialize_size = stable_total_aggregated_statistics_.serialize(tmp_dynamic_array, 0);
        tmp_dynamic_array.writeBinaryFile(0, fs_ptr, tmp_serialize_size);
        size += tmp_serialize_size;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    std::string TotalStatisticsTracker::toString() const
    {
        std::ostringstream oss;
        for (uint32_t slot_idx = 0; slot_idx < perslot_total_aggregated_statistics_.size(); slot_idx++)
        {
            oss << std::endl << "[Time slot " << slot_idx << "]" << std::endl;
            oss << perslot_total_aggregated_statistics_[slot_idx].toString() << std::endl;
        }
        oss << std::endl << "[Stable]" << std::endl;
        oss << stable_total_aggregated_statistics_.toString();
        
        std::string total_statistics_string = oss.str();
        return total_statistics_string;
    }

    // Used by dump() to check filepath
    std::string TotalStatisticsTracker::checkFilepathForDump_(const std::string& filepath) const
    {
        assert(allow_update_ == true);

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
            oss << filepath << "." << random_number;
            tmp_filepath = oss.str();

            // Dump hints
            oss.clear(); // Clear error states
            oss.str(""); // Set content as empty string and reset read/write position as zero
            oss << "use a random file path " << tmp_filepath << " for statistics!";
            Util::dumpNormalMsg(kClassName, oss.str());
        }

        return tmp_filepath;
    }

    // Load per-slot/stable total aggregated statistics
    uint32_t TotalStatisticsTracker::load_(const std::string& filepath)
    {
        assert(allow_update_ == false);

        bool is_exist = Util::isFileExist(filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "statistics file " << filepath << " does not exist!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Open the existing binary file for total aggregated statistics
        std::fstream* fs_ptr = Util::openFile(filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Load per-slot/stable total aggregated statistics
        // Format: slotcnt + perslot_total_aggregated_statistics_ + stable_total_aggregated_statistics_
        uint32_t size = 0;
        // (1) slotcnt
        uint32_t slotcnt = 0;
        fs_ptr->read((char *)&slotcnt, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (2) perslot_total_aggregated_statistics_
        perslot_total_aggregated_statistics_.resize(slotcnt);
        for (uint32_t i = 0; i < slotcnt; i++)
        {
            DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
            tmp_dynamic_array.readBinaryFile(0, fs_ptr, ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
            uint32_t tmp_deserialize_size = perslot_total_aggregated_statistics_[i].deserialize(tmp_dynamic_array, 0);
            size += tmp_deserialize_size;
        }
        // (3) stable_total_aggregated_statistics_
        DynamicArray tmp_dynamic_array(ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        tmp_dynamic_array.readBinaryFile(0, fs_ptr, ClientAggregatedStatistics::getAggregatedStatisticsIOSize());
        uint32_t tmp_deserialize_size = stable_total_aggregated_statistics_.deserialize(tmp_dynamic_array, 0);
        size += tmp_deserialize_size;

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }
}