/*
 * WorkloadCLI: parse and process workload CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef WORKLOAD_CLI_H
#define WORKLOAD_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"

namespace covered
{
    class WorkloadCLI : virtual public CLIBase
    {
    public:
        WorkloadCLI();
        virtual ~WorkloadCLI();

        uint32_t getKeycnt() const;
        std::string getWorkloadName() const;
    private:
        static const std::string kClassName;

        void checkWorkloadName_() const;
        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        uint32_t keycnt_;
        std::string workload_name_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
    };
}

#endif