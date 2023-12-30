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

        uint64_t getCapacityBytes() const;
        // uint32_t getEdgecnt() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint64_t DEFAULT_CAPACITY_MB;
        //static const uint32_t DEFAULT_EDGECNT;

        static const std::string kClassName;

        void checkCapacityBytes_() const;
        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        bool is_to_cli_string_;

        uint64_t capacity_bytes_; // Scalability on per-edge memory bytes
        // uint32_t edgecnt_; // Scalability on the number of edge nodes
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
    };
}

#endif