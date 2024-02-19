#include "cli/dataset_loader_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const uint32_t DatasetLoaderCLI::DEFAULT_DATASET_LOADERCNT = 1;
    const uint32_t DatasetLoaderCLI::DEFAULT_CLOUD_IDX = 0;

    const std::string DatasetLoaderCLI::kClassName("DatasetLoaderCLI");

    DatasetLoaderCLI::DatasetLoaderCLI() : CloudCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        dataset_loadercnt_ = 0;
        cloud_idx_ = 0;
    }

    DatasetLoaderCLI::DatasetLoaderCLI(int argc, char **argv) : CloudCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    DatasetLoaderCLI::~DatasetLoaderCLI() {}

    uint32_t DatasetLoaderCLI::getDatasetLoadercnt() const
    {
        return dataset_loadercnt_;
    }

    uint32_t DatasetLoaderCLI::getCloudIdx() const
    {
        return cloud_idx_;
    }

    std::string DatasetLoaderCLI::toCliString()
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
            if (dataset_loadercnt_ != DEFAULT_DATASET_LOADERCNT)
            {
                oss << " --dataset_loadercnt " << dataset_loadercnt_;
            }
            if (cloud_idx_ != DEFAULT_CLOUD_IDX)
            {
                oss << " --cloud_idx " << cloud_idx_;
            }

            is_to_cli_string_ = true;
        }

        return oss.str();
    }

    void DatasetLoaderCLI::clearIsToCliString()
    {
        CloudCLI::clearIsToCliString();

        is_to_cli_string_ = false;
        return;
    }

    void DatasetLoaderCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CloudCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("dataset_loadercnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_DATASET_LOADERCNT), "the total number of dataset loaders")
                ("cloud_idx", boost::program_options::value<uint32_t>()->default_value(DEFAULT_CLOUD_IDX), "the index of current cloud node")
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

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t dataset_loadercnt = argument_info_["dataset_loadercnt"].as<uint32_t>();
            uint32_t cloud_idx = argument_info_["cloud_idx"].as<uint32_t>();

            // Store client CLI parameters for dynamic configurations
            dataset_loadercnt_ = dataset_loadercnt;
            cloud_idx_ = cloud_idx;

            is_set_param_and_config_ = true;
        }

        return;
    }

    void DatasetLoaderCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            CloudCLI::verifyAndDumpCliParameters_(main_class_name);

            verifyIntegrity_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Dataset loader count: " << dataset_loadercnt_ << std::endl;
            oss << "Cloud index: " << cloud_idx_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void DatasetLoaderCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CloudCLI::createRequiredDirectories_(main_class_name);

            bool is_createdir_for_rocksdb = false;
            if (main_class_name == Util::DATASET_LOADER_MAIN_NAME)
            {
                is_createdir_for_rocksdb = true;
            }
            else if (main_class_name == Util::CLIUTIL_MAIN_NAME)
            {
                is_createdir_for_rocksdb = false;
            }
            else
            {
                std::ostringstream oss;
                oss << main_class_name << " should NOT use DatasetLoaderCLI!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }

            if (is_createdir_for_rocksdb)
            {
                std::string dirpath = Util::getCloudRocksdbBasedirForWorkload(getKeycnt(), getWorkloadName());
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

    void DatasetLoaderCLI::verifyIntegrity_() const
    {
        assert(dataset_loadercnt_ > 0);

        return;
    }
}