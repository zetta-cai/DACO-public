/*
 * PropagationCLI: parse and process propagation CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef PROPAGATION_CLI_H
#define PROPAGATION_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli_latency_info.h"
#include "cli/cli_base.h"

namespace covered
{
    class PropagationCLI : virtual public CLIBase
    {
    public:
        PropagationCLI();
        virtual ~PropagationCLI();

        CLILatencyInfo getCLILatencyInfo() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const std::string DEFAULT_PROPAGATION_LATENCY_DISTNAME;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_LBOUND_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_RBOUND_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_LBOUND_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_RBOUND_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_LBOUND_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_RBOUND_US;

        static const std::string kClassName;

        void verifyPropagationLatencyDistname_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        bool is_to_cli_string_;

        // NOTE: avg is for constant distribution; left/right bound is for uniform distribution
        CLILatencyInfo cli_latency_info_; // Distribution, avg, left bound, and right bound of each type of propagation latency
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
    };
}

#endif