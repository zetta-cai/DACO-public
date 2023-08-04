/*
 * DatasetLoaderCLI: parse and process dataset loader CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLIENT_CLI_H
#define CLIENT_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"
#include "common/cli/cloud_cli.h"
#include "common/cli/workload_cli.h"

namespace covered
{
    class DatasetLoaderCLI : virtual public CloudCLI, virtual public WorkloadCLI
    {
    public:
        DatasetLoaderCLI();
        DatasetLoaderCLI(int argc, char **argv);
        virtual ~DatasetLoaderCLI();

        uint32_t getDatasetLoadercnt() const;
        uint32_t getCloudIdx() const;
    private:
        static const std::string kClassName;

        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        uint32_t dataset_loadercnt_;
        uint32_t cloud_idx_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif