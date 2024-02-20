/*
 * EvaluatorCLI: parse and process evaluator CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EVALUATOR_CLI_H
#define EVALUATOR_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/client_cli.h"
#include "cli/edge_cli.h"

namespace covered
{
    class EvaluatorCLI : virtual public ClientCLI, virtual public EdgeCLI
    {
    public:
        EvaluatorCLI();
        EvaluatorCLI(int argc, char **argv);
        virtual ~EvaluatorCLI();

        uint32_t getWarmupReqcntScale() const;
        uint32_t getWarmupMaxDurationSec() const;
        uint32_t getStresstestDurationSec() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint32_t DEFAULT_WARMUP_REQCNT_SCALE;
        static const uint32_t DEFAULT_WARMUP_MAX_DURATION_SEC;
        static const uint32_t DEFAULT_STRESSTEST_DURATION_SEC;

        static const std::string kClassName;

        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;

        uint32_t warmup_reqcnt_scale_;
        uint32_t warmup_max_duration_sec_;
        uint32_t stresstest_duration_sec_;
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif