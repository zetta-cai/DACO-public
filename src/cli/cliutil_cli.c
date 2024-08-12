#include "cli/cliutil_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string CliutilCLI::kClassName("CliutilCLI");

    CliutilCLI::CliutilCLI(int argc, char **argv) : DatasetLoaderCLI(), SingleNodeCLI(), TracePreprocessorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    CliutilCLI::~CliutilCLI() {}

    std::string CliutilCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << DatasetLoaderCLI::toCliString();
            oss << SingleNodeCLI::toCliString();
            oss << TracePreprocessorCLI::toCliString();

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void CliutilCLI::clearIsToCliString()
    {
        DatasetLoaderCLI::clearIsToCliString();
        SingleNodeCLI::clearIsToCliString();
        TracePreprocessorCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

    void CliutilCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            DatasetLoaderCLI::addCliParameters_();
            SingleNodeCLI::addCliParameters_();
            TracePreprocessorCLI::addCliParameters_();

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void CliutilCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            DatasetLoaderCLI::setParamAndConfig_(main_class_name);
            SingleNodeCLI::setParamAndConfig_(main_class_name);
            TracePreprocessorCLI::setParamAndConfig_(main_class_name);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void CliutilCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            DatasetLoaderCLI::verifyAndDumpCliParameters_(main_class_name);
            SingleNodeCLI::verifyAndDumpCliParameters_(main_class_name);
            TracePreprocessorCLI::verifyAndDumpCliParameters_(main_class_name);

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void CliutilCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            DatasetLoaderCLI::createRequiredDirectories_(main_class_name);
            SingleNodeCLI::createRequiredDirectories_(main_class_name);
            TracePreprocessorCLI::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }

        return;
    }
}