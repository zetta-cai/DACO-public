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

    void TotalStatisticsTracker::aggregateClientStatistics_(const uint32_t& clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs)
    {
        // Get per-slot/stable client aggregated statistics from all clients
        uint32_t max_slot_cnt = 0;
        std::vector<std::vector<ClientAggregatedStatistics>> perclient_perslot_client_aggregated_statistics;
        std::vector<ClientAggregatedStatistics> perclient_stable_client_aggregated_statistics;
        perclient_perslot_client_aggregated_statistics.resize(clientcnt);
        perclient_stable_client_aggregated_statistics.resize(clientcnt);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            assert(client_statistics_tracker_ptrs[client_idx] != NULL);
            perclient_perslot_client_aggregated_statistics[client_idx] = client_statistics_tracker_ptrs[client_idx]->getPerslotClientAggregatedStatistics();
            perclient_stable_client_aggregated_statistics[client_idx] = client_statistics_tracker_ptrs[client_idx]->getStableClientAggregatedStatistics();

            if (perclient_perslot_client_aggregated_statistics[client_idx].size() > max_slot_cnt)
            {
                max_slot_cnt = perclient_perslot_client_aggregated_statistics[client_idx].size();
            }
        }

        // Aggregate per-slot client aggregated statistics
        perslot_total_aggregated_statistics_.resize(max_slot_cnt);
        for (uint32_t slot_idx = 0; slot_idx < max_slot_cnt; slot_idx++)
        {
            std::vector<ClientAggregatedStatistics*> curslot_client_aggregated_statistics_ptrs;
            curslot_client_aggregated_statistics_ptrs.resize(clientcnt);
            for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
            {
                uint32_t clienti_slot_cnt = perclient_perslot_client_aggregated_statistics[client_idx].size();
                if (slot_idx < clienti_slot_cnt)
                {
                    curslot_client_aggregated_statistics_ptrs[client_idx] = &perclient_perslot_client_aggregated_statistics[client_idx][slot_idx];
                }
                else
                {
                    // NOTE: use the last slot of client client_idx to approximate slot slot_idx
                    curslot_client_aggregated_statistics_ptrs[client_idx] = &perclient_perslot_client_aggregated_statistics[client_idx][clienti_slot_cnt-1];
                }
            }
            perslot_total_aggregated_statistics_[slot_idx] = TotalAggregatedStatistics(curslot_client_aggregated_statistics_ptrs);
        }

        // Aggregate stable client aggregated statistics
        std::vector<ClientAggregatedStatistics*> stable_client_aggregated_statistics_ptrs;
        stable_client_aggregated_statistics_ptrs.resize(clientcnt);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            stable_client_aggregated_statistics_ptrs[client_idx] = &perclient_stable_client_aggregated_statistics[client_idx];
        }
        stable_total_aggregated_statistics_ = TotalAggregatedStatistics(stable_client_aggregated_statistics_ptrs);

        return;
    }
}