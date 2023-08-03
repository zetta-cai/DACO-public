#include "common/cli/evaluator_cli.h"

#include "common/config.h"
#include "common/param/evaluator_param.h"
#include "common/util.h"

namespace covered
{
    const std::string EvaluatorCLI::kClassName("EvaluatorCLI");

    EvaluatorCLI::EvaluatorCLI(int argc, char **argv) : ClientCLI(), EdgeCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    EvaluatorCLI::~EvaluatorCLI() {}

    void EvaluatorCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            ClientCLI::addCliParameters_();
            EdgeCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("max_warmup_duration_sec", boost::program_options::value<uint32_t>()->default_value(10), "duration of warmup phase (seconds)")
                ("stresstest_duration_sec", boost::program_options::value<uint32_t>()->default_value(10), "duration of stresstest phase (seconds)")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void EvaluatorCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            ClientCLI::setParamAndConfig_(main_class_name);
            EdgeCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t max_warmup_duration_sec = argument_info_["max_warmup_duration_sec"].as<uint32_t>();
            uint32_t stresstest_duration_sec = argument_info_["stresstest_duration_sec"].as<uint32_t>();

            // Store client CLI parameters for dynamic configurations and mark ClientParam as valid
            EvaluatorParam::setParameters(max_warmup_duration_sec, stresstest_duration_sec);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EvaluatorCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        bool is_createdir_for_evaluator_statistics = false;
        if (main_class_name == CommonParam::SIMULATOR_MAIN_NAME)
        {
            is_createdir_for_evaluator_statistics = true;
        }
        else
        {
            // TODO: create directories for different prototype roles
        }

        if (is_createdir_for_evaluator_statistics)
        {
            std::string dirpath = Util::getStatisticsDirpath();
            bool is_dir_exist = Util::isDirectoryExist(dirpath);
            if (!is_dir_exist)
            {
                // Create directory for client statistics
                std::ostringstream oss;
                oss << "create directory " << dirpath << " for statistics";
                Util::dumpNormalMsg(kClassName, oss.str());

                Util::createDirectory(dirpath);
            }
        }

        return;
    }
}