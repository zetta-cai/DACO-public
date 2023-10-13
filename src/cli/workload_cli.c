#include "cli/workload_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string WorkloadCLI::kClassName("WorkloadCLI");

    WorkloadCLI::WorkloadCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false)
    {
        keycnt_ = 0;
        workload_name_ = "";
    }

    WorkloadCLI::~WorkloadCLI() {}

    uint32_t WorkloadCLI::getKeycnt() const
    {
        return keycnt_;
    }

    std::string WorkloadCLI::getWorkloadName() const
    {
        return workload_name_;
    }

    void WorkloadCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("keycnt", boost::program_options::value<uint32_t>()->default_value(10000), "the total number of keys")
                ("workload_name", boost::program_options::value<std::string>()->default_value(Util::FACEBOOK_WORKLOAD_NAME), "workload name (e.g., facebook)")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void WorkloadCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t keycnt = argument_info_["keycnt"].as<uint32_t>();
            std::string workload_name = argument_info_["workload_name"].as<std::string>();

            // Store workload CLI parameters for dynamic configurations
            keycnt_ = keycnt;
            workload_name_ = workload_name;
            checkWorkloadName_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void WorkloadCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            CLIBase::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Key count (dataset size): " << keycnt_ << std::endl;
            oss << "Workload name: " << workload_name_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void WorkloadCLI::checkWorkloadName_() const
    {
        if (workload_name_ != Util::FACEBOOK_WORKLOAD_NAME)
        {
            std::ostringstream oss;
            oss << "workload name " << workload_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void WorkloadCLI::verifyIntegrity_() const
    {
        assert(keycnt_ > 0);

        return;
    }
}