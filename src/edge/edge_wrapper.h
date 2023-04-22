/*
 * EdgeWrapper: the edge node to receive requests and send responses.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <string>

#include "edge/edge_wrapper.h"

namespace covered
{
    class EdgeWrapper
    {
    public:
        static void* launchEdge(void* local_edge_param_ptr);

        EdgeWrapper(EdgeParam* local_edge_param_ptr);
        ~EdgeWrapper();

        void start();
    private:
        static const std::string kClassName;

        EdgeParam* local_edge_param_ptr_;
    };
}

#endif