#include "cli/edge_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgeCLI::kClassName("EdgeCLI");

    EdgeCLI::EdgeCLI() : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        cache_name_ = "";
        capacity_bytes_ = 0;
        hash_name_ = "";
        percacheserver_workercnt_ = 0;
    }

    EdgeCLI::EdgeCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    EdgeCLI::~EdgeCLI() {}

    std::string EdgeCLI::getCacheName() const
    {
        return cache_name_;
    }

    uint64_t EdgeCLI::getCapacityBytes() const
    {
        return capacity_bytes_;
    }

    std::string EdgeCLI::getHashName() const
    {
        return hash_name_;
    }

    uint32_t EdgeCLI::getPercacheserverWorkercnt() const
    {
        return percacheserver_workercnt_;
    }

    void EdgeCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            EdgescaleCLI::addCliParameters_();
            PropagationCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("cache_name", boost::program_options::value<std::string>()->default_value(Util::LRU_CACHE_NAME), "cache name (e.g., lfu, lru, and covered)")
                ("capacity_mb", boost::program_options::value<uint64_t>()->default_value(1024), "total cache capacity (including data and metadata) in units of MiB")
                ("hash_name", boost::program_options::value<std::string>()->default_value(Util::MMH3_HASH_NAME, "the type of consistent hashing for DHT (e.g., mmh3)"))
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
            EdgescaleCLI::setParamAndConfig_(main_class_name);
            PropagationCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            std::string cache_name = argument_info_["cache_name"].as<std::string>();
            uint64_t capacity_bytes = MB2B(argument_info_["capacity_mb"].as<uint64_t>()); // In units of bytes
            std::string hash_name = argument_info_["hash_name"].as<std::string>();
            uint32_t percacheserver_workercnt = argument_info_["percacheserver_workercnt"].as<uint32_t>();

            // Store edgecnt CLI parameters for dynamic configurations
            cache_name_ = cache_name;
            checkCacheName_();
            capacity_bytes_ = capacity_bytes;
            hash_name_ = hash_name;
            checkHashName_();
            percacheserver_workercnt_ = percacheserver_workercnt;
            verifyIntegrity_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EdgeCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            EdgescaleCLI::dumpCliParameters_();
            PropagationCLI::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Cache name: " << cache_name_ << std::endl;
            oss << "Capacity (bytes): " << capacity_bytes_ << std::endl;
            oss << "Hash name: " << hash_name_ << std::endl;
            oss << "Per-cache-server worker count:" << percacheserver_workercnt_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void EdgeCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            is_create_required_directories_ = true;
        }
        return;
    }

    void EdgeCLI::checkCacheName_() const
    {
        if (cache_name_ != Util::LFU_CACHE_NAME && cache_name_ != Util::LRU_CACHE_NAME && cache_name_ != Util::DATASET_LOADER_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "cache name " << cache_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeCLI::checkHashName_() const
    {
        if (hash_name_ != Util::MMH3_HASH_NAME)
        {
            std::ostringstream oss;
            oss << "hash name " << hash_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeCLI::verifyIntegrity_() const
    {
        assert(capacity_bytes_ > 0);
        assert(percacheserver_workercnt_ > 0);
        
        return;
    }
}