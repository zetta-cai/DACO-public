/*
 * EdgeParam: parameters to launch an edge node.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_PARAM_H
#define EDGE_PARAM_H

#include <string>

namespace covered
{
    class EdgeParam
    {
    public:
        EdgeParam();
        EdgeParam(uint32_t global_edge_idx);
        ~EdgeParam();

        const EdgeParam& operator=(const EdgeParam& other);

        uint32_t getGlobalEdgeIdx();
    private:
        static const std::string kClassName;

        // UDP port for receiving requests is global_edge_recvreq_startport + global_edge_idx
        uint32_t global_edge_idx_;
    };
}

#endif