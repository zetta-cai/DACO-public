/*
 * Util: provide utilities, e.g., I/O.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>

#include <boost/system.hpp>
#include <boost/filesystem.hpp>

namespace covered
{
    class Util
    {
    public:
        // I/O
        static void dumpDebugMsg(const std::string& class_name, const std::string& debug_message);
        static void dumpWarnMsg(const std::string& class_name, const std::string& warn_message);
        static void dumpErrorMsg(const std::string& class_name, const std::string& error_message);
        static bool isFileExist(const std::string& filepath);
    private:
        static const std::string class_name_;
    };
}

#endif