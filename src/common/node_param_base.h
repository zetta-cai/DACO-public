/*
 * NodeParamBase: parameters to launch a client/edge/cloud node.
 * 
 * By Siyuan Sheng (2023.07.04).
 */

#ifndef NODE_PARAM_BASE_H
#define NODE_PARAM_BASE_H

#include <atomic>
#include <string>

namespace covered
{
    class NodeParamBase
    {
    public:
        NodeParamBase();
        NodeParamBase(const uint32_t& node_idx, const bool& is_running);
        ~NodeParamBase();

        const NodeParamBase& operator=(const NodeParamBase& other);

        uint32_t getNodeIdx() const;

        bool isNodeRunning() const;
        void setNodeRunning();
        void resetNodeRunning();
    private:
        static const std::string kClassName;

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // CPU cache coherence: MSI protocol.
        // CPU cache consistency: volatile.
        volatile std::atomic<bool> node_running_;
    protected:
        uint32_t node_idx_;
    };
}

#endif