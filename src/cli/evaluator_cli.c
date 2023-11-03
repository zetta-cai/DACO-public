#include "cli/evaluator_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string EvaluatorCLI::kClassName("EvaluatorCLI");

    EvaluatorCLI::EvaluatorCLI() : ClientCLI(), EdgeCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
    }

    EvaluatorCLI::EvaluatorCLI(int argc, char **argv) : ClientCLI(), EdgeCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    EvaluatorCLI::~EvaluatorCLI() {}

    uint32_t EvaluatorCLI::getMaxWarmupDurationSec() const
    {
        return max_warmup_duration_sec_;
    }

    uint32_t EvaluatorCLI::getStresstestDurationSec() const
    {
        return stresstest_duration_sec_;
    }

    void EvaluatorCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            ClientCLI::addCliParameters_();
            EdgeCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("max_warmup_duration_sec", boost::program_options::value<uint32_t>()->default_value(30), "duration of warmup phase (seconds)")
                ("stresstest_duration_sec", boost::program_options::value<uint32_t>()->default_value(30), "duration of stresstest phase (seconds)")
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

            // Store client CLI parameters for dynamic configurations
            max_warmup_duration_sec_ = max_warmup_duration_sec;
            stresstest_duration_sec_ = stresstest_duration_sec;

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EvaluatorCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            ClientCLI::dumpCliParameters_();
            EdgeCLI::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Max warmup duration seconds: " << max_warmup_duration_sec_ << std::endl;
            oss << "Stresstest duration seconds: " << stresstest_duration_sec_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void EvaluatorCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            ClientCLI::createRequiredDirectories_(main_class_name);
            EdgeCLI::createRequiredDirectories_(main_class_name);

            bool is_createdir_for_evaluator_statistics = false;
            if (main_class_name == Util::SIMULATOR_MAIN_NAME || main_class_name == Util::EVALUATOR_MAIN_NAME)
            {
                is_createdir_for_evaluator_statistics = true;
            }
            else if (main_class_name == Util::TOTAL_STATISTICS_LOADER_MAIN_NAME)
            {
                is_createdir_for_evaluator_statistics = false;
            }
            else
            {
                std::ostringstream oss;
                oss << main_class_name << " should NOT use EvaluatorCLI!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }

            if (is_createdir_for_evaluator_statistics)
            {
                std::string dirpath = Util::getEvaluatorStatisticsDirpath(this);
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

            is_create_required_directories_ = true;
        }

        return;
    }
}