#include "cli/edgescale_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgescaleCLI::kClassName("EdgescaleCLI");

    EdgescaleCLI::EdgescaleCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false)
    {
        capacity_bytes_ = 0;
        edgecnt_ = 0;
    }

    EdgescaleCLI::~EdgescaleCLI() {}

    uint64_t EdgescaleCLI::getCapacityBytes() const
    {
        assert(capacity_bytes_ > 0);
        return capacity_bytes_;
    }

    uint32_t EdgescaleCLI::getEdgecnt() const
    {
        return edgecnt_;
    }

    void EdgescaleCLI::checkCapacityBytes_() const
    {
        assert(capacity_bytes_ > 0);
        // uint64_t min_capacity_bytes = MB2B(Config::getMinCapacityMB());
        // if (capacity_bytes_ < min_capacity_bytes)
        // {
        //     std::ostringstream oss;
        //     oss << "capacity (" << capacity_bytes_ << " bytes) should >= the minimum capacity (" << min_capacity_bytes << " bytes) -> please use a larger capacity_mb in CLI!";
        //     Util::dumpErrorMsg(kClassName, oss.str());
        //     exit(1);
        // }
        return;
    }

    void EdgescaleCLI::verifyIntegrity_() const
    {
        assert(edgecnt_ > 0);
        
        return;
    }

    void EdgescaleCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("capacity_mb", boost::program_options::value<uint64_t>()->default_value(1024), "total cache capacity (including data and metadata) in units of MiB")
                ("edgecnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of edge nodes")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void EdgescaleCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint64_t capacity_bytes = MB2B(argument_info_["capacity_mb"].as<uint64_t>()); // In units of bytes
            uint32_t edgecnt = argument_info_["edgecnt"].as<uint32_t>();

            // Store edgescale CLI parameters for dynamic configurations
            capacity_bytes_ = capacity_bytes;
            checkCapacityBytes_();
            edgecnt_ = edgecnt;
            verifyIntegrity_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EdgescaleCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            CLIBase::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Capacity (bytes): " << capacity_bytes_ << std::endl;
            oss << "Edge count: " << edgecnt_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }
}