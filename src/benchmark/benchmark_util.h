/*
 * BenchmarkUtil: utils for benchmarking.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef BENCHMARK_UTIL_H
#define BENCHMARK_UTIL_H

#include <string>

namespace covered
{
    class BenchmarkUtil
    {
    public:
        static uint16_t getLocalClientWorkloadStartport(const uint32_t& global_client_idx);
        static std::string getLocalEdgeNodeIpstr(const uint32_t& global_client_idx);
        static uint32_t getGlobalWorkerIdx(const uint32_t& global_client_idx, const uint32_t local_worker_idx);
    private:
        static const std::string kClassName;
    };
}

#endif