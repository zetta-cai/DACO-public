/*
 * CommonParam: store CLI parameters for common dynamic configurations.
 *
 * NOTE: different Params should NOT have overlapped fields, and each Param will be set at most once.
 *
 * NOTE: all parameters should be passed into each instance when launching threads, except those with no effect on results (see comments of "// NO effect on results").
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef COMMON_PARAM_H
#define COMMON_PARAM_H

#include <string>

namespace covered
{
    class CommonParam
    {
    public:
        // For main class name
        static const std::string SIMULATOR_MAIN_NAME;
        //static const std::string STATISTICS_AGGREGATOR_MAIN_NAME;
        static const std::string TOTAL_STATISTICS_LOADER_MAIN_NAME;
        static const std::string CLIENT_MAIN_NAME;
        static const std::string EDGE_MAIN_NAME;
        static const std::string CLOUD_MAIN_NAME;

        static void setParameters(const std::string& main_class_name, const bool& is_single_node, const std::string& config_filepath, const bool& is_debug, const bool& track_event);

        static std::string getMainClassName();
        static bool isSingleNode();
        static std::string getConfigFilepath();
        static bool isDebug();
        static bool isTrackEvent();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkMainClassName_();

        static void checkIsValid_();

        static std::string main_class_name_;
        static bool is_valid_;
        
        static bool is_single_node_; // Come from CLI::multinode
        static std::string config_filepath_; // NO effect on results
        static bool is_debug_; // Come from CLI::debug // NO effect on results
        static bool track_event_; // NO effect on results
    };
}

#endif