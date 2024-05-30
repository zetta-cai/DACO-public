#include "cli/workload_cli.h"

#include "common/config.h"
#include "common/util.h"
#include "workload/fbphoto_workload_wrapper.h"

namespace covered
{
    const uint32_t WorkloadCLI::DEFAULT_KEYCNT = 1000000; // 1M workload items by default
    const std::string WorkloadCLI::DEFAULT_WORKLOAD_NAME = "facebook"; // NOTE: NOT use UTil::FACEBOOK_WORKLOAD_NAME due to undefined initialization order of C++ static variables
    const float WorkloadCLI::DEFAULT_ZIPF_ALPHA = 0.0; // Zipfian distribution parameter

    const std::string WorkloadCLI::kClassName("WorkloadCLI");

    WorkloadCLI::WorkloadCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_to_cli_string_(false)
    {
        keycnt_ = 0;
        workload_name_ = "";
        zipf_alpha_ = 0.0;
    }

    WorkloadCLI::~WorkloadCLI() {}

    uint32_t WorkloadCLI::getKeycnt() const
    {
        return keycnt_;
    }

    std::string WorkloadCLI::getWorkloadName() const
    {
        return workload_name_;
    }

    float WorkloadCLI::getZipfAlpha() const
    {
        return zipf_alpha_;
    }

    std::string WorkloadCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);

            oss << CLIBase::toCliString();
            if (keycnt_ != DEFAULT_KEYCNT)
            {
                oss << " --keycnt " << keycnt_;
            }
            if (workload_name_ != DEFAULT_WORKLOAD_NAME)
            {
                oss << " --workload_name " << workload_name_;
            }
            if (zipf_alpha_ != DEFAULT_ZIPF_ALPHA)
            {
                oss << " --zipf_alpha " << zipf_alpha_;
            }

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void WorkloadCLI::clearIsToCliString()
    {
        CLIBase::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

    void WorkloadCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            std::string keycnt_descstr = "the number of unique keys (dataset size; NOT affect " + Util::getReplayedWorkloadHintstr() + " and " + Util::FBPHOTO_WORKLOAD_NAME + ")";
            // OBSOLETE: self-reproduced Facebook photo caching trace is too skewed to be realistic; replayed traces cannot get reasonable results due to incorrect trace partitioning without geographical information
            // Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME + ", " + Util::WIKIPEDIA_TEXT_WORKLOAD_NAME + ", " + Util::FBPHOTO_WORKLOAD_NAME
            std::string workload_name_descstr = "workload name (e.g., " + Util::FACEBOOK_WORKLOAD_NAME + ", " + Util::ZIPF_FACEBOOK_WORKLOAD_NAME + ", " + Util::ZIPF_WIKIPEDIA_IMAGE_WORKLOAD_NAME + ", and " + Util::ZIPF_WIKIPEDIA_TEXT_WORKLOAD_NAME + ")";
            std::string zipf_alpha_descstr = "Zipf's law alpha (ONLY for the workload of " + Util::ZIPF_FACEBOOK_WORKLOAD_NAME + ")"; // NOTE: zipf_alpha does NOT affect zifp_wikiimage and zipf_wikitext, whose Zipfian constants are fixed

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("keycnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_KEYCNT), keycnt_descstr.c_str())
                ("workload_name", boost::program_options::value<std::string>()->default_value(DEFAULT_WORKLOAD_NAME), workload_name_descstr.c_str())
                ("zipf_alpha", boost::program_options::value<float>()->default_value(DEFAULT_ZIPF_ALPHA), zipf_alpha_descstr.c_str())
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void WorkloadCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t keycnt = argument_info_["keycnt"].as<uint32_t>();
            std::string workload_name = argument_info_["workload_name"].as<std::string>();
            float zipf_alpha = argument_info_["zipf_alpha"].as<float>();

            // Store workload CLI parameters for dynamic configurations
            if (main_class_name == Util::TRACE_PREPROCESSOR_MAIN_NAME) // (OBSOLETE due to no geographical information of replayed workloads) Replayed workloads are NOT preprocessed yet
            {
                keycnt = 0;
            }
            else if (Util::isReplayedWorkload(workload_name)) // (OBSOLETE due to no geographical information of replayed workloads) Already preprocessed for replayed workloads
            {
                keycnt = Config::getTraceKeycnt(workload_name);
            }
            else if (workload_name == Util::FBPHOTO_WORKLOAD_NAME) // (OBSOLETE due to not open-sourced and too skewed)
            {
                keycnt = FbphotoWorkloadWrapper::FBPHOTO_WORKLOAD_DATASET_KEYCNT;
            }
            keycnt_ = keycnt;
            workload_name_ = workload_name;
            zipf_alpha_ = zipf_alpha;

            is_set_param_and_config_ = true;
        }

        return;
    }

    void WorkloadCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            CLIBase::verifyAndDumpCliParameters_(main_class_name);

            checkWorkloadName_();
            verifyIntegrity_(main_class_name);

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Key count (dataset size): " << keycnt_ << std::endl;
            oss << "Workload name: " << workload_name_;
            if (workload_name_ == Util::ZIPF_FACEBOOK_WORKLOAD_NAME)
            {
                oss << " (Zipf's law alpha: " << zipf_alpha_ << ")";
            }
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void WorkloadCLI::checkWorkloadName_() const
    {
        // OBSOLETE: self-reproduced Facebook photo caching trace is too skewed to be realistic; replayed traces cannot get reasonable results due to incorrect trace partitioning without geographical information
        // workload_name_ != Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME && workload_name_ != Util::WIKIPEDIA_TEXT_WORKLOAD_NAME && workload_name_ != Util::FBPHOTO_WORKLOAD_NAME
        if (workload_name_ != Util::FACEBOOK_WORKLOAD_NAME && workload_name_ != Util::ZIPF_FACEBOOK_WORKLOAD_NAME && workload_name_ != Util::ZIPF_WIKIPEDIA_IMAGE_WORKLOAD_NAME && workload_name_ != Util::ZIPF_WIKIPEDIA_TEXT_WORKLOAD_NAME)
        {
            std::ostringstream oss;
            oss << "workload name " << workload_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void WorkloadCLI::verifyIntegrity_(const std::string& main_class_name) const
    {
        if (main_class_name == Util::TRACE_PREPROCESSOR_MAIN_NAME)
        {
            if (Util::isReplayedWorkload(workload_name_))
            {
                assert(keycnt_ == 0); // Set by setParamAndConfig_()
            }
            else
            {
                std::ostringstream oss;
                oss << "workload " << workload_name_ << " is NOT replayed and NO need to run trace preprocessor!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        if (main_class_name != Util::TRACE_PREPROCESSOR_MAIN_NAME && keycnt_ == 0) // Already preprocessed yet with invalid key count
        {
            if (Util::isReplayedWorkload(workload_name_)) // From Config for replayed workloads
            {
                std::ostringstream oss;
                oss << "please run trace_preprocessor and update config.json for " << workload_name_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
            else // From CLI for non-replayed workloads
            {
                std::ostringstream oss;
                oss << "invalid key count " << keycnt_ << " for non-replayed workload " << workload_name_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        return;
    }
}