/*
 * CLI: parse and process CLI parameters for static/dynamic actions and static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.05.26).
 */

#ifndef CLI_H
#define CLI_H

namespace covered
{
    class CLI
    {
    public:
        static void parseAndProcessCliParameters(const std::string& main_class_name);
    private:
        static void parseCliParameters_();
        static void processCliParameters_(const std::string& main_class_name);
    };
}

#endif