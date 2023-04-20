/*
 * PhysicalClientUtil: utils for physical clients.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef PHYSICAL_CLIENT_UTIL_H
#define PHYSICAL_CLIENT_UTIL_H

#include "benchmark/physical_client_param.h"

namespace covered
{
    class PhysicalClientUtil
    {
    public:
        static uint16_t getLocalClientWorkloadStartport(uint32_t global_client_idx);
        static std::string getLocalEdgeNodeIpstr(uint32_t global_client_idx);

        static void* launchPhysicalClient(void* physical_client_param);
    private:
        static std::string kClassName;
    };
}

#endif