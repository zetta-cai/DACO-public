/*
 * CacheServerMetadataUpdateProcessor: process metadata update requests partitioned from cache server.
 *
 * NOTE: beacon edge node issues metadata update requests to enable/disable is_neighbor_cached flags in cached metadata if the cached object becomes non-single/single cache copy.
 * 
 * By Siyuan Sheng (2023.11.28).
 */

#ifndef CACHE_SERVER_METADATA_UPDATE_PROCESSOR_H
#define CACHE_SERVER_METADATA_UPDATE_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerMetadataUpdateProcessor
    {
    public:
        static void* launchCacheServerMetadataUpdateProcessor(void* cache_server_metadata_update_processor_param_ptr);
    
        CacheServerMetadataUpdateProcessor(CacheServerProcessorParam* cache_server_metadata_update_processor_param_ptr);
        virtual ~CacheServerMetadataUpdateProcessor();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processMetadataUpdateRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process metadata update request

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string instance_name_;
        CacheServerProcessorParam* cache_server_metadata_update_processor_param_ptr_;
    };
}

#endif