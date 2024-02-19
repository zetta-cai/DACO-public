#include "cli/cloud_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string CloudCLI::DEFAULT_CLOUD_STORAGE = "hdd"; // NOTE: NOT use UTil::HDD_NAME due to undefined initialization order of C++ static variables

    const std::string CloudCLI::kClassName("CloudCLI");

    CloudCLI::CloudCLI() : PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        cloud_storage_ = "";
    }

    CloudCLI::CloudCLI(int argc, char **argv) : PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    CloudCLI::~CloudCLI() {}

    std::string CloudCLI::getCloudStorage() const
    {
        return cloud_storage_;
    }

    std::string CloudCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << PropagationCLI::toCliString();
            oss << WorkloadCLI::toCliString();
            if (cloud_storage_ != DEFAULT_CLOUD_STORAGE)
            {
                oss << " --cloud_storage " << cloud_storage_;
            }

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void CloudCLI::clearIsToCliString()
    {
        PropagationCLI::clearIsToCliString();
        WorkloadCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
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
                ("cloud_storage", boost::program_options::value<std::string>()->default_value(DEFAULT_CLOUD_STORAGE), "type of cloud storage (e.g., hdd)")
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

            is_set_param_and_config_ = true;
        }

        return;
    }

    void CloudCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            PropagationCLI::verifyAndDumpCliParameters_(main_class_name);
            WorkloadCLI::verifyAndDumpCliParameters_(main_class_name);

            checkCloudStorage_();

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