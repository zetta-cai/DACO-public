/*
 * Config: load JSON config file for static configurations.
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
        // Key strings of JSON config file for static configurations (only used by Config)
        static const std::string VERSION_KEYSTR;
        static const std::string FACEBOOK_CONFIG_FILEPATH_KEYSTR;
        static const std::string GLOBAL_EDGE_RECVREQ_STARTPORT_KEYSTR;

        static void loadConfig();

        static std::string getVersion();
        static uint16_t getGlobalEdgeRecvreqStartport();
        static std::string getFacebookConfigFilepath();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void parseJsonFile_(const std::string& config_filepath);
        static boost::json::key_value_pair* find_(const std::string& key);
        static void checkIsValid_();

        static bool is_valid_;
        static boost::json::object json_object_;
        static std::string version_;
        static std::string facebook_config_filepath_;
        static uint16_t global_edge_recvreq_startport_;
    };
}

#endif