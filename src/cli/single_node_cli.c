#include "cli/single_node_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const uint32_t SingleNodeCLI::DEFAULT_SIMULATOR_WORKLOADCNT = 1; // NOTE: workloadcnt just used to fix memory issue of large-scale simulation (yet each client worker still has an individual workload worker), yet NOT affect simulator results (already verified)!
    const uint32_t SingleNodeCLI::DEFAULT_SIMULATOR_RANDOMNESS = 0;

    const std::string SingleNodeCLI::kClassName("SingleNodeCLI");

    SingleNodeCLI::SingleNodeCLI() : CloudCLI(), EvaluatorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        simulator_workloadcnt_ = 0;
        simulator_randomness_ = 0;
    }

    SingleNodeCLI::SingleNodeCLI(int argc, char **argv) : CloudCLI(), EvaluatorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    SingleNodeCLI::~SingleNodeCLI() {}

    uint32_t SingleNodeCLI::getSimulatorWorkloadcnt() const
    {
        return simulator_workloadcnt_;
    }

    uint32_t SingleNodeCLI::getSimulatorRandomness() const
    {
        return simulator_randomness_;
    }

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
            if (simulator_workloadcnt_ != DEFAULT_SIMULATOR_WORKLOADCNT)
            {
                oss << " --simulator_workloadcnt " << simulator_workloadcnt_;
            }
            if (simulator_randomness_ != DEFAULT_SIMULATOR_RANDOMNESS)
            {
                oss << " --simulator_randomness " << simulator_randomness_;
            }

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

            // Dynamic configurations for single-node prototype/simulator
            argument_desc_.add_options()
                ("simulator_workloadcnt", boost::program_options::value<uint32_t>(&simulator_workloadcnt_)->default_value(DEFAULT_SIMULATOR_WORKLOADCNT), "Number of workload generators (ONLY for single-node simulator)")
                ("simulator_randomness", boost::program_options::value<uint32_t>(&simulator_randomness_)->default_value(DEFAULT_SIMULATOR_RANDOMNESS), "Randomness (ONLY for single-node simulator)");

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

            // ONLY for single-node simulator
            uint32_t simulator_workloadcnt = argument_info_["simulator_workloadcnt"].as<uint32_t>();
            uint32_t simulator_randomness = argument_info_["simulator_randomness"].as<uint32_t>();

            // Store edgecnt CLI parameters for dynamic configurations
            simulator_workloadcnt_ = simulator_workloadcnt;
            simulator_randomness_ = simulator_randomness;

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

            verifyIntegrity_();

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

    void SingleNodeCLI::verifyIntegrity_() const
    {
        // ONLY for single-node simulator
        assert(simulator_workloadcnt_ > 0);
        
        return;
    }


}