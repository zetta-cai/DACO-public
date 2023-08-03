/*
 * ClientParam: store CLI parameters for client dynamic configurations.
 *
 * See NOTEs in CommonParam.
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
        static void setParameters(const uint32_t& clientcnt, const bool& is_warmup_speedup, const uint32_t& opcnt, const uint32_t& perclient_workercnt);

        static uint32_t getClientcnt();
        static bool isWarmupSpeedup();
        static uint32_t getOpcnt();
        static uint32_t getPerclientWorkercnt();

        static bool isValid();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void verifyIntegrity_();

        static void checkIsValid_();

        static bool is_valid_;

        static uint32_t clientcnt_;
        static bool is_warmup_speedup_; // Come from CLI::disable_warmup_speedup
        static uint32_t opcnt_;
        static uint32_t perclient_workercnt_;
    };
}

#endif