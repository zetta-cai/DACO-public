/*
 * EdgeCLI: parse and process edge CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGE_CLI_H
#define EDGE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/edgescale_cli.h"
#include "common/cli/propagation_cli.h"

namespace covered
{
    class EdgeCLI : virtual public EdgescaleCLI, virtual public PropagationCLI
    {
    public:
        EdgeCLI();
        EdgeCLI(int argc, char **argv);
        virtual ~EdgeCLI();

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        std::string getHashName() const;
        uint32_t getPercacheserverWorkercnt() const;
    private:
        static const std::string kClassName;

        void checkCacheName_() const;
        void checkHashName_() const;
        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        std::string cache_name_;
        uint64_t capacity_bytes_;
        std::string hash_name_;
        uint32_t percacheserver_workercnt_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif