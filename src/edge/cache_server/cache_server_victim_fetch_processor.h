/*
 * CacheServerVictimFetchProcessor: process victim fetch requests partitioned from cache server.
 * 
 * By Siyuan Sheng (2023.10.04).
 */

#ifndef CACHE_SERVER_VICTIM_FETCH_PROCESSOR_H
#define CACHE_SERVER_VICTIM_FETCH_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerVictimFetchProcessor
    {
    public:
        static void* launchCacheServerVictimFetchProcessor(void* cache_server_victim_fetch_processor_param_ptr);
    
        CacheServerVictimFetchProcessor(CacheServerProcessorParam* cache_server_victim_fetch_processor_param_ptr);
        virtual ~CacheServerVictimFetchProcessor();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processVictimFetchRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process victim fetch request

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string instance_name_;
        CacheServerProcessorParam* cache_server_victim_fetch_processor_param_ptr_;
    };
}

#endif