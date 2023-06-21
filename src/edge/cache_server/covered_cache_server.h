/*
 * CoveredCacheServer: cache server in edge for COVERED.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef COVERED_CACHE_SERVER_H
#define COVERED_CACHE_SERVER_H

#include <string>

#include "edge/cache_server/cache_server_base.h"

namespace covered
{
    class CoveredCacheServer : public CacheServerBase
    {
    public:
        CoveredCacheServer(EdgeWrapper* edge_wrapper_ptr);
        virtual ~CoveredCacheServer();
    private:
        static const std::string kClassName;

        // Data requests

        // Return if edge node is finished
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const override;
        virtual void triggerIndependentAdmission_(const Key& key, const Value& value) const override;

        // Const variable
        std::string instance_name_;
    };
}

#endif