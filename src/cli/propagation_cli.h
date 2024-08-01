/*
 * PropagationCLI: parse and process propagation CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef PROPAGATION_CLI_H
#define PROPAGATION_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cli_base.h"

namespace covered
{
    class PropagationCLI : virtual public CLIBase
    {
    public:
        PropagationCLI();
        virtual ~PropagationCLI();

        uint32_t getPropagationLatencyClientedgeAvgUs() const;
        uint32_t getPropagationLatencyCrossedgeAvgUs() const;
        uint32_t getPropagationLatencyEdgecloudAvgUs() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US;
        static const uint32_t DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US;

        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        bool is_to_cli_string_;

        uint32_t propagation_latency_clientedge_avg_us_; // Average RTT between client and edge
        uint32_t propagation_latency_crossedge_avg_us_; // Average RTT between edge and neighbor
        uint32_t propagation_latency_edgecloud_avg_us_; // Average RTT between edge and cloud
    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
    };
}

#endif