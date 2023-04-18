/*
 * Config: parse config files in json format.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

/*
 * Config: load config file for static configurations.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <boost/json.hpp>

namespace covered
{
    class Config
    {
    public:
        static const std::string VERSION_KEYSTR;
        static const std::string GLOBAL_CLIENT_WORKLOAD_STARTPORT_KEYSTR;

        static void setConfig(const std::string& config_filepath);

        static std::string getVersion();
        static uint16_t getGlobalClientWorkloadStartport();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void parseJsonFile(const std::string& config_filepath);
        static boost::json::key_value_pair* find(const std::string& key);
        static void checkIsValid();

        static bool is_valid_;
        static std::string filepath_;
        static boost::json::object json_object_;
        static std::string version_;
        static uint16_t client_workload_startport_;
    };
}

#endif