#include "cli/trace_preprocessor_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string TracePreprocessorCLI::kClassName("TracePreprocessorCLI");

    TracePreprocessorCLI::TracePreprocessorCLI() : WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
    }

    TracePreprocessorCLI::TracePreprocessorCLI(int argc, char **argv) : WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    TracePreprocessorCLI::~TracePreprocessorCLI() {}

    std::string TracePreprocessorCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << WorkloadCLI::toCliString();

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void TracePreprocessorCLI::clearIsToCliString()
    {
        WorkloadCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

    void TracePreprocessorCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            WorkloadCLI::addCliParameters_();

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void TracePreprocessorCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            WorkloadCLI::setParamAndConfig_(main_class_name);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void TracePreprocessorCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            WorkloadCLI::verifyAndDumpCliParameters_(main_class_name);

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void TracePreprocessorCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            // NOTE: nothing to create for trace preprocessor

            is_create_required_directories_ = true;
        }

        return;
    }
}