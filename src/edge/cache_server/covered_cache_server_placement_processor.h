/*
 * CoveredCacheServerPlacementProcessorBase: the class for COVERED to process placement notification requests partitioned from cache server.
 * 
 * By Siyuan Sheng (2024.02.02).
 */

#ifndef COVERED_CACHE_SERVER_PLACEMENT_PROCESSOR_H
#define COVERED_CACHE_SERVER_PLACEMENT_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_placement_processor_base.h"

namespace covered
{
    class CoveredCacheServerPlacementProcessor : public CacheServerPlacementProcessorBase
    {
    public:
        CoveredCacheServerPlacementProcessor(CacheServerProcessorParam* cache_server_placement_processor_param_ptr);
        virtual ~CoveredCacheServerPlacementProcessor();
    private:
        static const std::string kClassName;

        virtual bool processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr) override; // Process remote placement notification request (return if edge node is finished)
        virtual bool processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item) override; // Process local cache admission (return if edge node is finished)

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif