/*
 * CacheServerPlacementProcessorBaseBase: the base class to process placement notification requests partitioned from cache server.
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef CACHE_SERVER_PLACEMENT_PROCESSOR_BASE_H
#define CACHE_SERVER_PLACEMENT_PROCESSOR_BASE_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerPlacementProcessorBase
    {
    public:
        static void* launchCacheServerPlacementProcessor(void* cache_server_placement_processor_param_ptr);
    
        CacheServerPlacementProcessorBase(CacheServerProcessorParam* cache_server_placement_processor_param_ptr);
        virtual ~CacheServerPlacementProcessorBase();

        void start();
    protected:
        // Const variables
        CacheServerProcessorParam* cache_server_placement_processor_param_ptr_;

        bool processLocalCacheAdmissionInternal_(const LocalCacheAdmissionItem& local_cache_admission_item); // Return if edge node is finished
    private:
        static const std::string kClassName;

        bool processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process remote placement notification request (return if edge node is finished)

        virtual bool processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item) = 0; // Process local cache admission (return if edge node is finished)

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string base_instance_name_;

        // NOTE: destination addresses for sending control requests come from beacon edge index

        // For receiving control responses
        NetworkAddr edge_cache_server_placement_processor_recvrsp_source_addr_; // Used by cache server placement processor to send back control responses (const individual variable)
        UdpMsgSocketServer* edge_cache_server_placement_processor_recvrsp_socket_server_ptr_; // Used by cache server placement processor to receive control responses from beacon server (non-const individual variable)
    };
}

#endif