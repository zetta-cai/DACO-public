/*
 * BasicCacheServerPlacementProcessorBase: the class for baselines to process placement notification requests partitioned from cache server.
 * 
 * By Siyuan Sheng (2024.02.02).
 */

#ifndef BASIC_CACHE_SERVER_PLACEMENT_PROCESSOR_H
#define BASIC_CACHE_SERVER_PLACEMENT_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_placement_processor_base.h"

namespace covered
{
    class BasicCacheServerPlacementProcessor : public CacheServerPlacementProcessorBase
    {
    public:
        BasicCacheServerPlacementProcessor(CacheServerProcessorParam* cache_server_placement_processor_param_ptr);
        virtual ~BasicCacheServerPlacementProcessor();
    private:
        static const std::string kClassName;

        virtual bool processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item) override; // Process local cache admission (return if edge node is finished)

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif