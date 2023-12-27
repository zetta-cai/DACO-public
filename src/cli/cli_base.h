/*
 * CLIBase: the base class to parse and process common CLI parameters for static/dynamic actions and static/dynamic configurations.
 *
 * NOTE: CLIs only track dynamic parameters related with evaluation results.
 * 
 * NOTE: different CLIs should NOT have overlapped parameters, and each virtual function will be invoked at most once.
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLI_BASE_H
#define CLI_BASE_H

#include <string>

#include <boost/program_options.hpp>

namespace covered
{
    class CLIBase
    {
    public:
        CLIBase();
        virtual ~CLIBase();

        uint32_t getClientcnt() const;
        uint32_t getEdgecnt() const;

        void parseAndProcessCliParameters(int argc, char **argv);
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        // NOTE: JUST for evaluation trick as we may launch nodes with different roles under the same physical machine due to limited devices, while clientcnt/edgecnt is ONLY required by ClientCLI/EdgescaleCLI if each machine plays a single role in practice
        uint32_t clientcnt_;
        uint32_t edgecnt_; // Scalability on the number of edge nodes
    protected:
        virtual void addCliParameters_();
        void parseCliParameters_(int argc, char **argv);
        virtual void setParamAndConfig_(const std::string& main_class_name);
        void processCliParameters_();
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) = 0;

        void verifyIntegrity_() const;

        boost::program_options::options_description argument_desc_;
        boost::program_options::variables_map argument_info_;
    };
}

#endif