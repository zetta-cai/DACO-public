#include "common/cli/edgescale_cli.h"

#include "common/config.h"
#include "common/param/edgescale_param.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgescaleCLI::kClassName("EdgescaleCLI");

    EdgescaleCLI::EdgescaleCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false)
    {
    }

    EdgescaleCLI::~EdgescaleCLI() {}

    void EdgescaleCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
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

            uint32_t edgecnt = argument_info_["edgecnt"].as<uint32_t>();

            // Store edgescale CLI parameters for dynamic configurations and mark EdgescaleParam as valid
            EdgescaleParam::setParameters(edgecnt);

            is_set_param_and_config_ = true;
        }

        return;
    }
}