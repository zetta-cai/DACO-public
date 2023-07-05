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

        const ClientParam& operator=(const ClientParam& other);
    private:
        static const std::string kClassName;
    };
}

#endif