/*
 * EdgeParam: parameters to launch an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_PARAM_H
#define EDGE_PARAM_H

#include <atomic>
#include <string>

#include "common/node_param_base.h"

namespace covered
{
    class EdgeParam : public NodeParamBase
    {
    public:
        EdgeParam();
        EdgeParam(uint32_t edge_idx);
        ~EdgeParam();

        const EdgeParam& operator=(const EdgeParam& other);
    private:
        static const std::string kClassName;
    };
}

#endif