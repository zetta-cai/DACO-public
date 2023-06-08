/*
 * CoveredEdgeWrapper: edge node for COVERED.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef COVERED_EDGE_WRAPPER_H
#define COVERED_EDGE_WRAPPER_H

#include <string>

#include "edge/edge_param.h"
#include "edge/edge_wrapper_base.h"

namespace covered
{
    class CoveredEdgeWrapper : public EdgeWrapperBase
    {
    public:
        CoveredEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~CoveredEdgeWrapper();
    private:
        static const std::string kClassName;

        // (1) Data requests

        // Return if edge node is finished
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr) override;

        virtual void triggerIndependentAdmission_(const Key& key, const Value& value) override;

        // (2) Control requests

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr) override;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) override;
    };
}

#endif