#include "cli/cloud_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string CloudCLI::kClassName("CloudCLI");

    CloudCLI::CloudCLI() : PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        cloud_storage_ = "";
    }

    CloudCLI::CloudCLI(int argc, char **argv) : PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    CloudCLI::~CloudCLI() {}

    std::string CloudCLI::getCloudStorage() const
    {
        return cloud_storage_;
    }

    void CloudCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            PropagationCLI::addCliParameters_();
            WorkloadCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("cloud_storage", boost::program_options::value<std::string>()->default_value(Util::HDD_NAME), "type of cloud storage (e.g., hdd)")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void CloudCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            PropagationCLI::setParamAndConfig_(main_class_name);
            WorkloadCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            std::string cloud_storage = argument_info_["cloud_storage"].as<std::string>();

            // Store cloud CLI parameters for dynamic configurations and mark CloudParam as valid
            cloud_storage_ = cloud_storage;
            checkCloudStorage_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void CloudCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            PropagationCLI::dumpCliParameters_();
            WorkloadCLI::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Cloud storage: " << cloud_storage_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void CloudCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            is_create_required_directories_ = true;
        }
        return;
    }

    void CloudCLI::checkCloudStorage_() const
    {
        if (cloud_storage_ != Util::HDD_NAME)
        {
            std::ostringstream oss;
            oss << "cloud storage " << cloud_storage_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
}