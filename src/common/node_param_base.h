/*
 * NodeParamBase: parameters to launch a client/edge/cloud node (thread safe).
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
        static const std::string CLIENT_NODE_ROLE;
        static const std::string EDGE_NODE_ROLE;
        static const std::string CLOUD_NODE_ROLE;

        NodeParamBase();
        NodeParamBase(const std::string& node_role, const uint32_t& node_idx, const bool& is_running);
        ~NodeParamBase();

        std::string getNodeRole() const;

        bool isNodeRunning() const;
        void setNodeRunning();
        void resetNodeRunning();

        uint32_t getNodeIdx() const;

        const NodeParamBase& operator=(const NodeParamBase& other);
    private:
        static const std::string kClassName;

        std::string node_role_; // const shared variable

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // CPU cache coherence: MSI protocol.
        // CPU cache consistency: volatile.
        volatile std::atomic<bool> node_running_; // thread safe
    protected:
        uint32_t node_idx_; // const shared variable
    };
}

#endif