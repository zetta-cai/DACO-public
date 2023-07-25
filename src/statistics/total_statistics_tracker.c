#include "statistics/total_statistics_tracker.h"

#include <assert.h>
//#include <cstring> // memset
#include <sstream>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string TotalStatisticsTracker::kClassName("TotalStatisticsTracker");
    
    TotalStatisticsTracker::TotalStatisticsTracker(const uint32_t& clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs)
    {
        aggregateClientStatistics_(clientcnt, client_statistics_tracker_ptrs);
    }
        
    TotalStatisticsTracker::~TotalStatisticsTracker() {}

    std::string TotalStatisticsTracker::toString() const
    {
        std::ostringstream oss;
        for (uint32_t i = 0; i < perslot_total_aggregated_statistics_.size(); i++)
        {
            oss << "[Time slot " << i << "]" << std::endl;
            oss << perslot_total_aggregated_statistics_[i].toString() << std::endl;
        }
        oss << "[Stable]" << std::endl;
        oss << stable_total_aggregated_statistics_.toString();
        
        std::string total_statistics_string = oss.str();
        return total_statistics_string;
    }

    void TotalStatisticsTracker::aggregateClientStatistics_(const uint32_t& clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs)
    {
        // Get per-slot/stable client aggregated statistics from all clients
        uint32_t max_slot_cnt = 0;
        std::vector<std::vector<ClientAggregatedStatistics>> perclient_perslot_client_aggregated_statistics;
        std::vector<ClientAggregatedStatistics> perclient_stable_client_aggregated_statistics;
        perclient_perslot_client_aggregated_statistics.resize(clientcnt);
        perclient_stable_client_aggregated_statistics.resize(clientcnt);
        for (uint32_t i = i; i < clientcnt; i++)
        {
            assert(client_statistics_tracker_ptrs[i] != NULL);
            perclient_perslot_client_aggregated_statistics[i] = client_statistics_tracker_ptrs[i]->getPerslotClientAggregatedStatistics();
            perclient_stable_client_aggregated_statistics[i] = client_statistics_tracker_ptrs[i]->getStableClientAggregatedStatistics();

            if (perclient_perslot_client_aggregated_statistics[i].size() > max_slot_cnt)
            {
                max_slot_cnt = perclient_perslot_client_aggregated_statistics[i].size();
            }
        }

        // Aggregate per-slot client aggregated statistics
        perslot_total_aggregated_statistics_.resize(max_slot_cnt);
        for (uint32_t j = 0; j < max_slot_cnt; j++)
        {
            std::vector<ClientAggregatedStatistics*> curslot_client_aggregated_statistics_ptrs;
            curslot_client_aggregated_statistics_ptrs.resize(clientcnt);
            for (uint32_t i = 0; i < clientcnt; i++)
            {
                curslot_client_aggregated_statistics_ptrs.push_back(&perclient_perslot_client_aggregated_statistics[i][j]);
            }
            perslot_total_aggregated_statistics_[j] = TotalAggregatedStatistics(curslot_client_aggregated_statistics_ptrs);
        }

        // Aggregate stable client aggregated statistics
        std::vector<ClientAggregatedStatistics*> stable_client_aggregated_statistics_ptrs;
        for (uint32_t i = 0; i < clientcnt; i++)
        {
            stable_client_aggregated_statistics_ptrs.push_back(&perclient_stable_client_aggregated_statistics[i]);
        }
        stable_total_aggregated_statistics_ = TotalAggregatedStatistics(stable_client_aggregated_statistics_ptrs);

        return;
    }
}