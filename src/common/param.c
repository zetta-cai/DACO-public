#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"
#include "common/param.h"

namespace covered
{
    const std::string Param::kClassName("Param");

    bool Param::is_valid_ = false;
    std::string Param::config_filepath_ = "";
    bool Param::is_debug_ = false;
    uint32_t Param::keycnt_ = 0;
    uint32_t Param::opcnt_ = 0;
    uint32_t Param::clientcnt_ = 0;
    uint32_t Param::perclient_workercnt_ = 0;

    void Param::setParameters(const bool& is_simulation, const std::string& config_filepath, const bool& is_debug, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& clientcnt, const uint32_t& perclient_workercnt)
    {
        is_simulation_ = is_simulation;
        config_filepath_ = config_filepath;
        is_debug_ = is_debug;
        keycnt_ = keycnt;
        opcnt_ = opcnt;
        clientcnt_ = clientcnt;
        perclient_workercnt_ = perclient_workercnt;

        is_valid_ = true;
        return;
    }

    bool Param::isSimulation()
    {
        checkIsValid();
        return is_simulation_;
    }

    std::string Param::getConfigFilepath()
    {
        checkIsValid();
        return config_filepath_;
    }

    bool Param::isDebug()
    {
        checkIsValid();
        return is_debug_;
    }

    uint32_t Param::getKeycnt()
    {
        checkIsValid();
        return keycnt_;
    }

    uint32_t Param::getOpcnt()
    {
        checkIsValid();
        return opcnt_;
    }

    uint32_t Param::getClientcnt()
    {
        checkIsValid();
        return clientcnt_;
    }

    uint32_t Param::getPerclientWorkercnt()
    {
        checkIsValid();
        return perclient_workercnt_;
    }

    std::string Param::toString()
    {
        checkIsValid();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters]" << std::endl;
        oss << "Config file path: " << config_filepath_ << std::endl;
        oss << "Debug flag: " << (is_debug_?"true":"false") << std::endl;
        oss << "Key count: " << keycnt_ << std::endl;
        oss << "Operation count: " << opcnt_ << std::endl;
        oss << "Client count: " << clientcnt_ << std::endl;
        oss << "Per-client worker count: " << perclient_workercnt_;
        return oss.str();
        
    }

    void Param::checkIsValid()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid Param (CLI parameters have not been set)!");
            exit(-1);
        }
        return;
    }
}