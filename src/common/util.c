#include <sstream> // ostringstream
#include <iostream> // cerr

#include <boost/system.hpp>
#include <boost/filesystem.hpp>

#include "common/util.h"

namespace covered
{
    const int64_t Util::MAX_UINT16 = 65536;
    const std::string Util::LOCALHOST_IPSTR("127.0.0.1");
    const unsigned int Util::SLEEP_INTERVAL_US = 1 * 1000 * 1000; // 1s

    const std::string Util::kClassName("Util");

    void Util::dumpNormalMsg(const std::string& class_name, const std::string& normal_message)
    {
        std::cout << class_name << ": " << normal_message << std::endl;
        return;
    }

    void Util::dumpDebugMsg(const std::string& class_name, const std::string& debug_message)
    {
        // \033 means ESC character, 1 means bold, 32 means green foreground, 0 means reset, and m is end character
        std::cout << "\033[1;32m" << "[DEBUG] " << class_name << ": " << debug_message << std::endl << "\033[0m";
        return;
    }

    void Util::dumpWarnMsg(const std::string& class_name, const std::string& warn_message)
    {
        // \033 means ESC character, 1 means bold, 33 means yellow foreground, 0 means reset, and m is end character
        std::cout << "\033[1;33m" << "[WARN] " << class_name << ": " << warn_message << std::endl << "\033[0m";
        return;
    }

    void Util::dumpErrorMsg(const std::string& class_name, const std::string& error_message)
    {
        // \033 means ESC character, 1 means bold, 31 means red foreground, 0 means reset, and m is end character
        std::cerr << "\033[1;31m" << "[ERROR] " << class_name << ": " << error_message << std::endl << "\033[0m";
        return;
    }

    bool Util::isFileExist(const std::string& filepath)
    {
        // Get boost::file_status
        boost::filesystem::path boost_filepath(filepath);
        boost::system::error_code boost_errcode;
        boost::filesystem::file_status boost_filestatus = boost::filesystem::status(boost_filepath, boost_errcode);

        // An error occurs
        bool is_error = bool(boost_errcode); // boost_errcode.m_val != 0
        if (is_error)
        {
            dumpWarnMsg(kClassName, boost_errcode.message());
            return false;
        }

        // File not exist
        if (!boost::filesystem::exists(boost_filestatus))
        {
            std::ostringstream oss;
            oss << filepath << " does not exist!";
            dumpWarnMsg(kClassName, oss.str());
            return false;
        }

        // Is a directory
        if (boost::filesystem::is_directory(boost_filestatus))
        {
            std::ostringstream oss;
            oss << filepath << " is a directory!";
            dumpWarnMsg(kClassName, oss.str());
            return false;
        }

        return true;
    }

    struct timespec Util::getCurrentTimespec()
    {
        struct timespec current_timespec;
        clock_gettime(CLOCK_REALTIME, &current_timespec);
        return current_timespec;
    }

    double Util::getDeltaTime(const struct timespec& current_timespec, const struct timespec& previous_timespec)
    {
        struct timespec delta_timespec;
        delta_timespec.tv_sec = current_timespec.tv_sec - previous_timespec.tv_sec;
		delta_timespec.tv_nsec = current_timespec.tv_nsec - previous_timespec.tv_nsec;
		if (delta_timespec.tv_nsec < 0)
        {
			delta_timespec.tv_sec--;
			delta_timespec.tv_nsec += 1000000000L;
		}

        double delta_time = delta_timespec.tv_sec * 1000 * 1000 + double(delta_timespec.tv_nsec) / 1000.0;
        return delta_time;
    }

    uint16_t Util::toUint16(const int64_t& val)
    {
        if (val >= 0 && val < MAX_UINT16)
        {
            uint16_t result = static_cast<uint16_t>(val);
            return result;
        }
        else
        {
            std::ostringstream oss;
            oss << "cannot convert " << val << " (> " << MAX_UINT16 << ") to uint16_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }
}