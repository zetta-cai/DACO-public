/*
 * EdgeParam: parameters to launch an edge node.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_PARAM_H
#define EDGE_PARAM_H

#include <atomic>
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

        bool isEdgeRunning();
        void setEdgeRunning();
        void resetEdgeRunning();

        uint32_t getGlobalEdgeIdx();
    private:
        static const std::string kClassName;

         volatile std::atomic<bool> local_edge_running_;

        // UDP port for receiving requests is global_edge_recvreq_startport + global_edge_idx
        uint32_t global_edge_idx_;
    };
}

#endif