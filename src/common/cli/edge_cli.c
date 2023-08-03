#include "common/cli/edge_cli.h"

#include "common/config.h"
#include "common/param/edge_param.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgeCLI::kClassName("EdgeCLI");

    EdgeCLI::EdgeCLI() : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
    }

    EdgeCLI::EdgeCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    EdgeCLI::~EdgeCLI() {}

    void EdgeCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            EdgescaleCLI::addCliParameters_();
            PropagationCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("cache_name", boost::program_options::value<std::string>()->default_value(EdgeParam::LRU_CACHE_NAME), "cache name")
                ("capacity_mb", boost::program_options::value<uint64_t>()->default_value(1024), "total cache capacity (including data and metadata) in units of MiB")
                ("hash_name", boost::program_options::value<std::string>()->default_value(EdgeParam::MMH3_HASH_NAME, "the type of consistent hashing for DHT"))
                ("percacheserver_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each cache server")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void EdgeCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            std::string cache_name = argument_info_["cache_name"].as<std::string>();
            uint64_t capacity_bytes = MB2B(argument_info_["capacity_mb"].as<uint64_t>()); // In units of bytes
            std::string hash_name = argument_info_["hash_name"].as<std::string>();
            uint32_t percacheserver_workercnt = argument_info_["percacheserver_workercnt"].as<uint32_t>();

            // Store edgecnt CLI parameters for dynamic configurations and mark EdgeParam as valid
            EdgeParam::setParameters(cache_name, capacity_bytes, hash_name, percacheserver_workercnt);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EdgeCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CLIBase::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }
        return;
    }
}