/*
 * ClientParam: store CLI parameters for client dynamic configurations.
 *
 * NOTE: different Params should NOT have overlapped fields, and each Param will be set at most once.
 *
 * NOTE: all parameters should be passed into each instance when launching threads, except those with no effect on results (see comments of "// NO effect on results").
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLIENT_PARAM_H
#define CLIENT_PARAM_H

#include <string>

namespace covered
{
    class ClientParam
    {
    public:
        static void setParameters(const uint32_t& clientcnt, const bool& is_warmup_speedup, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt);

        static uint32_t getClientcnt();
        static bool isWarmupSpeedup();
        static uint32_t getKeycnt();
        static uint32_t getOpcnt();
        static uint32_t getPerclientWorkercnt();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void verifyIntegrity_();

        static void checkIsValid_();

        static bool is_valid_;

        static uint32_t clientcnt_;
        static bool is_warmup_speedup_; // Come from CLI::disable_warmup_speedup
        static uint32_t keycnt_;
        static uint32_t opcnt_;
        static uint32_t perclient_workercnt_;
    };
}

#endif