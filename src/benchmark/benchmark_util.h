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
        static uint16_t getLocalClientWorkloadStartport(uint32_t global_client_idx);
        static std::string getLocalEdgeNodeIpstr(uint32_t global_client_idx);
    private:
        static std::string kClassName;
    };
}

#endif