/*
 * EdgeComponentParam: parameters to launch an edge component (e.g., edge cache server and edge beacon server).
 * 
 * By Siyuan Sheng (2024.03.02).
 */

#ifndef EDGE_COMPONENT_PARAM_H
#define EDGE_COMPONENT_PARAM_H

#include <atomic>
#include <string>

namespace covered
{
    class EdgeComponentParam;
}

#include "edge/edge_wrapper_base.h"
#include "common/subthread_param_base.h"

namespace covered
{
    class EdgeComponentParam : public SubthreadParamBase
    {
    public:
        EdgeComponentParam();
        EdgeComponentParam(EdgeWrapperBase* edge_wrapper_ptr);
        ~EdgeComponentParam();

        const EdgeComponentParam& operator=(const EdgeComponentParam& other);

        EdgeWrapperBase* getEdgeWrapperPtr() const;
    private:
        static const std::string kClassName;

        EdgeWrapperBase* edge_wrapper_ptr_;
    };
}

#endif