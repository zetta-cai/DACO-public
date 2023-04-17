#include <sstream> // ostringstream
#include <iostream> // cerr

#include "common/util.h"

const uint64_t covered::Util::MAX_UINT16 = 65536;

const std::string covered::Util::kClassName = "Util";

void covered::Util::dumpNormalMsg(const std::string& class_name, const std::string& normal_message)
{
    std::cout << class_name << ": " << normal_message << std::endl;
    return;
}

void covered::Util::dumpDebugMsg(const std::string& class_name, const std::string& debug_message)
{
    // \033 means ESC character, 1 means bold, 32 means green foreground, 0 means reset, and m is end character
    std::cout << "\033[1;32m" << "[DEBUG] " << class_name << ": " << debug_message << std::endl << "\033[0m";
    return;
}

void covered::Util::dumpWarnMsg(const std::string& class_name, const std::string& warn_message)
{
    // \033 means ESC character, 1 means bold, 33 means yellow foreground, 0 means reset, and m is end character
    std::cout << "\033[1;33m" << "[WARN] " << class_name << ": " << warn_message << std::endl << "\033[0m";
    return;
}

void covered::Util::dumpErrorMsg(const std::string& class_name, const std::string& error_message)
{
    // \033 means ESC character, 1 means bold, 31 means red foreground, 0 means reset, and m is end character
    std::cerr << "\033[1;31m" << "[ERROR] " << class_name << ": " << error_message << std::endl << "\033[0m";
    return;
}

bool covered::Util::isFileExist(const std::string& filepath)
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

uint16_t covered::Util::toUint16(const uint64_t& val)
{
    if (val <= MAX_UINT16)
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