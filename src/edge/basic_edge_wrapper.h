/*
 * BasicEdgeWrapper: basic edge node for baselines.
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

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr) override;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) override;
    };
}

#endif