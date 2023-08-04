/*
 * CloudCLI: parse and process cloud CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef CLOUD_CLI_H
#define CLOUD_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/propagation_cli.h"
#include "common/cli/workload_cli.h"

namespace covered
{
    class CloudCLI : virtual public PropagationCLI, virtual public WorkloadCLI
    {
    public:
        CloudCLI();
        CloudCLI(int argc, char **argv);
        virtual ~CloudCLI();

        std::string getCloudStorage() const;
    private:
        static const std::string kClassName;

        void checkCloudStorage_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        std::string cloud_storage_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif