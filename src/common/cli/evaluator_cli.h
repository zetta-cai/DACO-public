/*
 * EvaluatorCLI: parse and process evaluator CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EVALUATOR_CLI_H
#define EVALUATOR_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"
#include "common/cli/client_cli.h"
#include "common/cli/edge_cli.h"

namespace covered
{
    class EvaluatorCLI : virtual public ClientCLI, virtual public EdgeCLI
    {
    public:
        EvaluatorCLI(int argc, char **argv);
        virtual ~EvaluatorCLI();

        uint32_t getMaxWarmupDurationSec() const;
        uint32_t getStresstestDurationSec() const;
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        uint32_t max_warmup_duration_sec_;
        uint32_t stresstest_duration_sec_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif