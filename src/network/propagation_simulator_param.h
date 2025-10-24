/*
 * PropagationSimulatorParam: param shared by working threads of each node and the corresponding PropagationSimulator, which tracks messages to be issued after propagation latency (thread safe).
 *
 * NOTE: all PropagationItems in a PropagationSimulatorParam must have the same propagation latency, otherwise PropagationSimulator cannot simulate propagation latency for each item correctly.
 * 
 * By Siyuan Sheng (2023.07.03).
 */

#ifndef PROPAGATION_SIMULATOR_PARAM_H
#define PROPAGATION_SIMULATOR_PARAM_H

//#define DEBUG_PROPAGATION_SIMULATOR_PARAM

#include <random> // std::mt19937_64 and std::uniform_int_distribution
#include <string>
#include <time.h>

#include "concurrency/ring_buffer_impl.h"
#include "concurrency/rwlock.h"
#include "common/subthread_param_base.h"
#include "common/node_wrapper_base.h"
#include "network/propagation_item.h"

namespace covered
{
    class PropagationSimulatorParam : public SubthreadParamBase
    {
    public:
        enum class LinkDistType {
            UNIFORM,   // 均匀分布
            POISSON,   // 泊松分布
            PARETO     // 帕累托分布
        };
        // 嵌套类：存储单个链路的分布参数
        class LinkDistParams {
        public:
            LinkDistType type;         // 分布类型
            uint32_t l;            // 下界（所有分布共用）
            uint32_t r;            // 上界（所有分布共用）
            uint32_t lambda;         // 泊松分布的λ参数（仅Poisson使用）
            double k;              // 帕累托分布的形状参数k（仅Pareto使用）
            uint32_t uniform_l;
            uint32_t uniform_r;
            // 构造函数：默认初始化（避免未定义行为）
            LinkDistParams();
            
            // 带参构造：根据分布类型初始化对应参数
            LinkDistParams(LinkDistType t, uint32_t l_val, uint32_t r_val, uint32_t extra1, double extra2);

            LinkDistParams(std::string args, uint32_t seed);

        };
        const uint32_t PROPAGATION_SIMULATION_LINK_FACTOR = 1000;
        PropagationSimulatorParam();
        PropagationSimulatorParam(NodeWrapperBase* node_wrapper_ptr, const std::string& propagation_latency_distname, const uint32_t& propagation_latency_lbound_us, const uint32_t& propagation_latency_avg_us, const uint32_t& propagation_latency_rbound_us, const uint32_t& propagation_latency_random_seed, const uint32_t& propagation_item_buffer_size, const std::string& realnet_option, const std::vector<uint32_t> _p2p_latency_array = std::vector<uint32_t>()); 
        ~PropagationSimulatorParam();

        const NodeWrapperBase* getNodeWrapperPtr() const;
        
        bool push(MessageBase* message_ptr, const NetworkAddr& dst_addr);
        bool pop(PropagationItem& element); // Only invoked by PropagationSimulator

        // std::vector<std::vector<uint32_t>> propagation_latency_martix_;

        uint32_t genPropagationLatency(); // Atomic function
        uint32_t genPropagationLatency_of_j(int j);
        
        const PropagationSimulatorParam& operator=(const PropagationSimulatorParam& other);
    private:
        static const std::string kClassName;

        uint32_t genPropagationLatency_();
        uint32_t genPropagationLatency_of_j_(int j);
        uint32_t genPropagationLatency_of_j_rnd_(int j);
        // Const shared variables
        NodeWrapperBase* node_wrapper_ptr_;
        std::string propagation_latency_distname_;
        uint32_t propagation_latency_lbound_us_;
        uint32_t propagation_latency_avg_us_;
        uint32_t propagation_latency_rbound_us_;
        uint32_t propagation_latency_random_seed_;
        uint32_t propagation_latency_delta;
        std::string realnet_option_;
        std::string instance_name_;

        std::vector<uint32_t> p2p_latency_array;
        // Ensure the atomicity of all non-const variables due to multiple providers (all subthreads of a client/edge/cloud node)
        // NOTE: only use write lock of rwlock (similar to a mutex; yet not use mutex so as to utilize the debug info of rwlock), as ring buffer only allows one reader and one writer (both will modify indexes in ring buffer)
        Rwlock rwlock_for_propagation_item_buffer_;
        
        // Non-const variables shared by working threads of each ndoe and propagation simulator
        // NOTE: there will be multiple writers (processing threads of each componenet) yet a single reader (the corresponding propagation simulator) -> NOT need to acquire lock for pop()
        RingBuffer<PropagationItem>* propagation_item_buffer_ptr_;
        bool is_first_item_;
        struct timespec prev_timespec_;
        bool is_rnd_link;
        std::vector<uint32_t> link_seeds;
        std::vector<LinkDistParams> link_dist_params_;  // 索引对应链路ID
        std::vector<std::mt19937_64> link_randgens;
        uint32_t link_size;
        // Used to generate random latency distribution
        std::mt19937_64 propagation_latency_randgen_;
        std::uniform_int_distribution<uint32_t>* propagation_latency_dist_ptr_;
    };
}

#endif