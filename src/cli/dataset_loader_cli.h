/*
 * DatasetLoaderCLI: parse and process dataset loader CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef DATASET_LOADER_CLI_H
#define DATASET_LOADER_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cloud_cli.h"

namespace covered
{
    class DatasetLoaderCLI : virtual public CloudCLI
    {
    public:
        DatasetLoaderCLI();
        DatasetLoaderCLI(int argc, char **argv);
        virtual ~DatasetLoaderCLI();

        uint32_t getDatasetLoadercnt() const;
        uint32_t getCloudIdx() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint32_t DEFAULT_DATASET_LOADERCNT;
        static const uint32_t DEFAULT_CLOUD_IDX;

        static const std::string kClassName;

        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;

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