/*
 * CacheServerVictimFetchProcessor: process victim fetch requests partitioned from cache server.
 * 
 * By Siyuan Sheng (2023.10.04).
 */

#ifndef EDGE_CACHE_SERVER_CACHE_SERVER_VICTIM_FETCH_PROCESSOR_H
#define EDGE_CACHE_SERVER_CACHE_SERVER_VICTIM_FETCH_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerVictimFetchProcessor
    {
    public:
        static void* launchCacheServerVictimFetchProcessor(void* cache_serer_victim_fetch_processor_param_ptr);
    
        CacheServerVictimFetchProcessor(CacheServerProcessorParam* cache_serer_victim_fetch_processor_param_ptr);
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
        const CacheServerProcessorParam* cache_serer_victim_fetch_processor_param_ptr_;

        // NOTE: destination addresses for sending control requests come from beacon edge index

        // For receiving control responses
        NetworkAddr edge_cache_server_victim_fetch_processor_recvrsp_source_addr_; // Used by cache server victim fetch processor to send back control responses (const individual variable)
        UdpMsgSocketServer* edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_; // Used by cache server victim fetch processor to receive control responses from beacon server (non-const individual variable)
    };
}

#endif