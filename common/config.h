/*
 * Config: parse config files in json format.
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

        Config(const std::string& config_filepath);
        ~Config();

        std::string getVersion();

        std::string toString();
    private:
        static const std::string class_name_;

        void parseJsonFile(const std::string& config_filepath);
        boost::json::key_value_pair* find(const std::string& key);
        //void checkIsValid();

        std::string filepath_;
        //bool is_valid_;
        boost::json::object json_object_;

        std::string version_;
    };
}

#endif