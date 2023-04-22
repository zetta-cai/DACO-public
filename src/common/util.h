/*
 * Util: provide utilities, e.g., I/O and type conversion.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <time.h>

namespace covered
{
    class Util
    {
    public:
        static const int64_t MAX_UINT16;
        static const std::string LOCALHOST_IPSTR;
        static const unsigned int SLEEP_INTERVAL_US;
        static const uint32_t KVPAIR_GENERATION_SEED;

        // I/O
        static void dumpNormalMsg(const std::string& class_name, const std::string& normal_message);
        static void dumpDebugMsg(const std::string& class_name, const std::string& debug_message);
        static void dumpWarnMsg(const std::string& class_name, const std::string& warn_message);
        static void dumpErrorMsg(const std::string& class_name, const std::string& error_message);
        static bool isFileExist(const std::string& filepath);

        // Time measurement (in units of microseconds)
        static struct timespec getCurrentTimespec();
        static double getDeltaTime(const struct timespec& current_timespec, const struct timespec& previous_timespec);

        // Type conversion
        static uint16_t toUint16(const int64_t& val);
    private:
        static const std::string kClassName;
    };
}

#endif