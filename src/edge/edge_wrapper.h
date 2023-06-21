/*
 * EdgeWrapper: an edge node launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper.
 *
 * NOTE: all non-const shared variables in EdgeWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "edge/edge_param.h"

namespace covered
{
    class EdgeWrapper
    {
    public:
        EdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr, const uint32_t& capacity_bytes);
        virtual ~EdgeWrapper();

        void start();

        friend class CacheServerBase;
        friend class BasicCacheServer;
        friend class CoveredCacheServer;
        friend class BeaconServerBase;
    private:
        static const std::string kClassName;

        static void* launchBeaconServer_(void* edge_wrapper_ptr);
        static void* launchCacheServer_(void* edge_wrapper_ptr);
        static void* launchInvalidationServer_(void* edge_wrapper_ptr);

        // (2) Control requests

        // Return if edge node is finished
        bool processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr);
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const = 0;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) = 0;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) = 0;

        std::string instance_name_;

        // Const shared variables
        const std::string cache_name_;
        const EdgeParam* edge_param_ptr_; // Thread safe
        const uint32_t capacity_bytes_; // Come from Util::Param

        // Non-const shared variables (thread safe)
        mutable CacheWrapperBase* edge_cache_ptr_; // Data and metadata for local edge cache (thread safe)
        CooperationWrapperBase* cooperation_wrapper_ptr_; // Cooperation metadata (thread safe)
    };
}

#endif