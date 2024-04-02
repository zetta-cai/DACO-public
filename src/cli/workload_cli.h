/*
 * WorkloadCLI: parse and process workload CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef WORKLOAD_CLI_H
#define WORKLOAD_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cli_base.h"

namespace covered
{
    class WorkloadCLI : virtual public CLIBase
    {
    public:
        WorkloadCLI();
        virtual ~WorkloadCLI();

        uint32_t getKeycnt() const;
        std::string getWorkloadName() const;
        float getZipfAlpha() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint32_t DEFAULT_KEYCNT;
        static const std::string DEFAULT_WORKLOAD_NAME;
        static const float DEFAULT_ZIPF_ALPHA;

        static const std::string kClassName;

        void checkWorkloadName_() const;
        void verifyIntegrity_(const std::string& main_class_name) const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        bool is_to_cli_string_;

        uint32_t keycnt_;
        std::string workload_name_;
        float zipf_alpha_;
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
    };
}

#endif