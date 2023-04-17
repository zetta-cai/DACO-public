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

        Config(const std::string& config_filepath);
        ~Config();

        std::string getVersion();
        uint16_t getGlobalClientWorkloadStartport();

        std::string toString();
    private:
        static const std::string kClassName;

        void parseJsonFile(const std::string& config_filepath);
        boost::json::key_value_pair* find(const std::string& key);
        //void checkIsValid();

        std::string filepath_;
        //bool is_valid_;
        boost::json::object json_object_;

        std::string version_;
        uint16_t client_workload_startport_;
    };
}

#endif