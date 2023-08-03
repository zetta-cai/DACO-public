#include "common/cli/cloud_cli.h"

#include "common/config.h"
#include "common/param/cloud_param.h"
#include "common/util.h"

namespace covered
{
    const std::string CloudCLI::kClassName("CloudCLI");

    CloudCLI::CloudCLI() : PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
    }

    CloudCLI::CloudCLI(int argc, char **argv) : PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    CloudCLI::~CloudCLI() {}

    void CloudCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            PropagationCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("cloud_storage", boost::program_options::value<std::string>()->default_value(CloudParam::HDD_NAME), "type of cloud storage")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void CloudCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            std::string cloud_storage = argument_info_["cloud_storage"].as<std::string>();

            // Store cloud CLI parameters for dynamic configurations and mark CloudParam as valid
            CloudParam::setParameters(cloud_storage);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void CloudCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CLIBase::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }
        return;
    }
}