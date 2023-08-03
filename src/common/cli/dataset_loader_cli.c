#include "common/cli/dataset_loader_cli.h"

#include "common/config.h"
#include "common/param/common_param.h"
#include "common/param/dataset_loader_param.h"
#include "common/util.h"

namespace covered
{
    const std::string DatasetLoaderCLI::kClassName("DatasetLoaderCLI");

    DatasetLoaderCLI::DatasetLoaderCLI() : CloudCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
    }

    DatasetLoaderCLI::DatasetLoaderCLI(int argc, char **argv) : CloudCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    DatasetLoaderCLI::~DatasetLoaderCLI() {}

    void DatasetLoaderCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CloudCLI::addCliParameters_();
            WorkloadCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("dataset_loadercnt", boost::program_options::value<uint32_t>()->default_value(1), "the total number of dataset loaders")
                ("cloud_idx", boost::program_options::value<uint32_t>()->default_value(0), "the index of current cloud node")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void DatasetLoaderCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CloudCLI::setParamAndConfig_(main_class_name);
            WorkloadCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t dataset_loadercnt = argument_info_["dataset_loadercnt"].as<uint32_t>();
            uint32_t cloud_idx = argument_info_["cloud_idx"].as<uint32_t>();

            // Store client CLI parameters for dynamic configurations and mark ClientParam as valid
            DatasetLoaderParam::setParameters(dataset_loadercnt, cloud_idx);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void DatasetLoaderCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CloudCLI::createRequiredDirectories_(main_class_name);

            bool is_createdir_for_rocksdb = false;
            if (main_class_name == CommonParam::SIMULATOR_MAIN_NAME)
            {
                is_createdir_for_rocksdb = true;
            }
            else
            {
                // TODO: create directories for different prototype roles
            }

            if (is_createdir_for_rocksdb)
            {
                std::string dirpath = Util::getCloudRocksdbBasedirForWorkload();
                bool is_dir_exist = Util::isDirectoryExist(dirpath);
                if (!is_dir_exist)
                {
                    // Create directory for RocksDB KVS
                    std::ostringstream oss;
                    oss << "create directory " << dirpath << " for RocksDB";
                    Util::dumpNormalMsg(kClassName, oss.str());

                    Util::createDirectory(dirpath);
                }
            }

            is_create_required_directories_ = true;
        }
        return;
    }
}