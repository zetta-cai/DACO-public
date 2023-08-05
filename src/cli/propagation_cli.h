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

        uint32_t getPropagationLatencyClientedgeUs() const;
        uint32_t getPropagationLatencyCrossedgeUs() const;
        uint32_t getPropagationLatencyEdgecloudUs() const;
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        uint32_t propagation_latency_clientedge_us_; // 1/2 RTT between client and edge (bidirectional link)
        uint32_t propagation_latency_crossedge_us_; // 1/2 RTT between edge and neighbor (bidirectional link)
        uint32_t propagation_latency_edgecloud_us_; // 1/2 RTT between edge and cloud (bidirectional link)
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
    };
}

#endif