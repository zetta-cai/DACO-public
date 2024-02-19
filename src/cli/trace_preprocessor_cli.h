/*
 * TracePreprocessorCLI: parse and process trace preprocessor CLI parameters to preprocess corresponding trace files and get trace properties.
 *
 * NOTE: after trace preprocessor for new traces, you should update config.json and Config module if not yet.
 * 
 * By Siyuan Sheng (2024.02.17).
 */

#ifndef TRACE_PREPROCESSOR_CLI_H
#define TRACE_PREPROCESSOR_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/workload_cli.h"

namespace covered
{
    class TracePreprocessorCLI : virtual public WorkloadCLI
    {
    public:
        TracePreprocessorCLI(int argc, char **argv);
        virtual ~TracePreprocessorCLI();

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif