/*
 * BasicEdgeWrapper: basic edge node for baselines (thread safe).
 * 
 * By Siyuan Sheng (2023.06.07).
 */

#ifndef BASIC_EDGE_WRAPPER_H
#define BASIC_EDGE_WRAPPER_H

#include <string>

#include "edge/edge_param.h"
#include "edge/edge_wrapper_base.h"

namespace covered
{
    class BasicEdgeWrapper : public EdgeWrapperBase
    {
    public:
        BasicEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~BasicEdgeWrapper();
    private:
        static const std::string kClassName;

        // (1) Data requests

        // Return if edge node is finished
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const override;

        virtual void triggerIndependentAdmission_(const Key& key, const Value& value) const override;

        // (2) Control requests

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr) const override;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) override;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) override;

        std::string instance_name_;
    };
}

#endif