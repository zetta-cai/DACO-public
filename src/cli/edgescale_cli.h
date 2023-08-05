/*
 * EdgescaleCLI: parse and process edgescale CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGESCALE_CLI_H
#define EDGESCALE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cli_base.h"

namespace covered
{
    class EdgescaleCLI : virtual public CLIBase
    {
    public:
        EdgescaleCLI();
        virtual ~EdgescaleCLI();

        uint32_t getEdgecnt() const;
    private:
        static const std::string kClassName;

        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        uint32_t edgecnt_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
    };
}

#endif