/*
 * ClientCLI: parse and process client CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLIENT_CLI_H
#define CLIENT_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/edgescale_cli.h"
#include "cli/propagation_cli.h"
#include "cli/workload_cli.h"

namespace covered
{
    class ClientCLI : virtual public EdgescaleCLI, virtual public PropagationCLI, virtual public WorkloadCLI
    {
    public:
        ClientCLI();
        ClientCLI(int argc, char **argv);
        virtual ~ClientCLI();

        // uint32_t getClientcnt() const;
        bool isWarmupSpeedup() const;
        uint32_t getMaxEvalWorkloadLoadcntScale() const;
        uint32_t getPerclientOpcnt() const;
        uint32_t getPerclientWorkercnt() const;
        uint32_t getWarmupReqcntScale() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        //static const uint32_t DEFAULT_CLIENTCNT;
        static const bool DEFAULT_IS_WARMUP_SPEEDUP;
        static const uint32_t DEFAULT_MAX_EVAL_WORKLOAD_LOADCNT_SCALE;
        static const uint32_t DEFAULT_PERCLIENT_OPCNT;
        static const uint32_t DEFAULT_PERCLIENT_WORKERCNT;
        static const uint32_t DEFAULT_WARMUP_REQCNT_SCALE;

        static const std::string kClassName;

        void verifyIntegrity_(const std::string& main_class_name) const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;

        // uint32_t clientcnt_;
        bool is_warmup_speedup_; // Come from --disable_warmup_speedup
        uint32_t max_eval_workload_loadcnt_scale_; // Must > warmup_reqcnt_scale_
        uint32_t perclient_opcnt_;
        uint32_t perclient_workercnt_;
        uint32_t warmup_reqcnt_scale_; // Client needs it for per-client-worker warmup reqcnt limitation; evaluator needs it for switching warmup/stresstest phase
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif