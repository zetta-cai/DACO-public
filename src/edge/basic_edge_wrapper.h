/*
 * BasicEdgeWrapper: basic edge node for baselines.
 * 
 * By Siyuan Sheng (2023.06.07).
 */

#ifndef BASIC_EDGE_WRAPPER_H
#define BASIC_EDGE_WRAPPER_H

#include <string>

#include "edge/edge_wrapper_base.h"

namespace covered
{
    class BasicEdgeWrapper : public EdgeWrapperBase
    {
    public:
        BasicEdgeWrapper(const std::string& cache_name, EdgeParam* edge_param_ptr);
        ~BasicEdgeWrapper();
    private:
        static const std::string kClassName;

        // Return is_finish
        virtual bool processControlRequest_(MessageBase* request_ptr) override;
    };
}

#endif