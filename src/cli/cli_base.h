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
        std::string getRealnetOption() const;
        std::string getRealnetExpname() const;

        void parseAndProcessCliParameters(int argc, char **argv);

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const uint32_t DEFAULT_CLIENTCNT;
        static const uint32_t DEFAULT_EDGECNT;
        static const std::string DEFAULT_CONFIG_FILE;
        static const std::string DEFAULT_REALNET_OPTION;
        static const std::string DEFAULT_REALNET_EXPNAME;

        static const std::string kClassName;

        void verifyIntegrity_() const;
        void verifyRealnetParams_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        bool is_to_cli_string_;

        // NOTE: JUST for evaluation trick as we may launch nodes with different roles under the same physical machine due to limited devices (e.g., launching cloud needs to know clientcnt and edgecnt to calculate dedicated CPU cores), while clientcnt/edgecnt is ONLY required by ClientCLI/EdgescaleCLI if each machine plays a single role in practice
        uint32_t clientcnt_;
        uint32_t edgecnt_; // Scalability on the number of edge nodes
        std::string config_file_;
        std::string realnet_option_;
        std::string realnet_expname_;
    protected:
        virtual void addCliParameters_();
        void parseCliParameters_(int argc, char **argv);
        virtual void setParamAndConfig_(const std::string& main_class_name);
        void processCliParameters_();
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name);
        virtual void createRequiredDirectories_(const std::string& main_class_name) = 0;

        boost::program_options::options_description argument_desc_;
        boost::program_options::variables_map argument_info_;
    };
}

#endif