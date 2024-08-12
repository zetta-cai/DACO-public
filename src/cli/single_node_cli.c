#include "cli/single_node_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string SingleNodeCLI::kClassName("SingleNodeCLI");

    SingleNodeCLI::SingleNodeCLI() : CloudCLI(), EvaluatorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {}

    SingleNodeCLI::SingleNodeCLI(int argc, char **argv) : CloudCLI(), EvaluatorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    SingleNodeCLI::~SingleNodeCLI() {}

    std::string SingleNodeCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << CloudCLI::toCliString();
            oss << EvaluatorCLI::toCliString();

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void SingleNodeCLI::clearIsToCliString()
    {
        CloudCLI::clearIsToCliString();
        EvaluatorCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

    void SingleNodeCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CloudCLI::addCliParameters_();
            EvaluatorCLI::addCliParameters_();

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void SingleNodeCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CloudCLI::setParamAndConfig_(main_class_name);
            EvaluatorCLI::setParamAndConfig_(main_class_name);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void SingleNodeCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            CloudCLI::verifyAndDumpCliParameters_(main_class_name);
            EvaluatorCLI::verifyAndDumpCliParameters_(main_class_name);

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void SingleNodeCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CloudCLI::createRequiredDirectories_(main_class_name);
            EvaluatorCLI::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }

        return;
    }
}