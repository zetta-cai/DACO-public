/*
 * SingleNodeCLI: parse and process single-node (run all components including client, cache, cloud, and evaluator in a single node) CLI parameters dynamic configurations.
 *
 * NOTE: SingleNodeCLI is used by single-node prototype and single-node simulator, which just have implementation differences (the former has parallel nodes for absolute performance, while the latter simulates cache nodes one by one for hit ratios), yet the role is the same to run the caching system (client/cache/cloud/evaluator) precisely/approximately in a single node.
 * 
 * By Siyuan Sheng (2023.08.04).
 */

#ifndef SINGLE_NODE_CLI_H
#define SINGLE_NODE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cloud_cli.h"
#include "cli/evaluator_cli.h"

namespace covered
{
    class SingleNodeCLI : virtual public CloudCLI, virtual public EvaluatorCLI
    {
    public:
        SingleNodeCLI();
        SingleNodeCLI(int argc, char **argv);
        virtual ~SingleNodeCLI();

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