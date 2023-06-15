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
        EdgeParam(uint32_t edge_idx);
        ~EdgeParam();

        const EdgeParam& operator=(const EdgeParam& other);

        bool isEdgeRunning() const;
        void setEdgeRunning();
        void resetEdgeRunning();

        uint32_t getEdgeIdx() const;
    private:
        static const std::string kClassName;

         volatile std::atomic<bool> edge_running_;

        // UDP port for receiving requests is edge_recvreq_startport + edge_idx
        uint32_t edge_idx_;
    };
}

#endif