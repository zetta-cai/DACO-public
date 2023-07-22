/*
 * ClientParam: parameters to launch a client for simulation (thread safe).
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_PARAM_H
#define CLIENT_PARAM_H

#include <atomic>
#include <string>

#include "common/node_param_base.h"

namespace covered
{
    class ClientParam : public NodeParamBase
    {
    public:
        ClientParam();
        ClientParam(const uint32_t& client_idx);
        ~ClientParam();

        bool isWarmupPhase() const;
        void finishWarmupPhase();
        bool isStresstestPhase() const;
        void startStresstestPhase();

        const ClientParam& operator=(const ClientParam& other);
    private:
        static const std::string kClassName;

        std::atomic<bool> is_warmup_phase_;
        std::atomic<bool> is_stresstest_phase_;
    };
}

#endif