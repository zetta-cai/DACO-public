#include "common/param/common_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string CommonParam::SIMULATOR_MAIN_NAME("simulator");
    //const std::string CommonParam::STATISTICS_AGGREGATOR_MAIN_NAME("statistics_aggregator");
    const std::string CommonParam::TOTAL_STATISTICS_LOADER_MAIN_NAME("total_statistics_loader");
    const std::string CommonParam::CLIENT_MAIN_NAME("client");
    const std::string CommonParam::EDGE_MAIN_NAME("edge");
    const std::string CommonParam::CLOUD_MAIN_NAME("cloud");

    const std::string CommonParam::kClassName("CommonParam");

    std::string CommonParam::main_class_name_ = "";
    bool CommonParam::is_valid_ = false;
    
    bool CommonParam::is_single_node_ = true;
    bool CommonParam::is_debug_ = false;
    bool CommonParam::track_event_ = false;

    void CommonParam::setParameters(const std::string& main_class_name, const bool& is_single_node, const std::string& config_filepath, const bool& is_debug, const bool& track_event)
    {
        // NOTE: NOT rely on any other module
        if (is_valid_)
        {
            return; // NO need to set parameters once again
        }
        else
        {
            Util::dumpNormalMsg(kClassName, "invoke setParameters()!");
        }

        main_class_name_ = main_class_name;
        checkMainClassName_();
        is_single_node_ = is_single_node;
        config_filepath_ = config_filepath;
        is_debug_ = is_debug;
        track_event_ = track_event;

        is_valid_ = true;
        return;
    }

    std::string CommonParam::getMainClassName()
    {
        checkIsValid_();
        return main_class_name_;
    }

    bool CommonParam::isSingleNode()
    {
        checkIsValid_();
        return is_single_node_;
    }

    std::string CommonParam::getConfigFilepath()
    {
        checkIsValid_();
        return config_filepath_;
    }

    bool CommonParam::isDebug()
    {
        checkIsValid_();
        return is_debug_;
    }

    bool CommonParam::isTrackEvent()
    {
        checkIsValid_();
        return track_event_;
    }

    std::string CommonParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Is single node: " << (is_single_node_?"true":"false") << std::endl;
        oss << "Config filepath: " << config_filepath_ << std::endl;
        oss << "Debug flag: " << (is_debug_?"true":"false") << std::endl;
        oss << "Track event flag: " << (track_event_?"true":"false");

        return oss.str();
    }

    void CommonParam::checkMainClassName_()
    {
        // Obselete: STATISTICS_AGGREGATOR_MAIN_NAME
        if (main_class_name_ != SIMULATOR_MAIN_NAME && main_class_name_ != TOTAL_STATISTICS_LOADER_MAIN_NAME && main_class_name_ != CLIENT_MAIN_NAME && main_class_name_ != EDGE_MAIN_NAME && main_class_name_ != CLOUD_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "main class name " << main_class_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void CommonParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid CommonParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}