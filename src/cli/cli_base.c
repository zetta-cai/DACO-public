#include "cli/cli_base.h"

#include "common/config.h"
#include "common/util.h"
#include "network/udp_pkt_socket.h"

namespace covered
{
    const uint32_t CLIBase::DEFAULT_CLIENTCNT = 1;
    const uint32_t CLIBase::DEFAULT_EDGECNT = 1;
    const std::string CLIBase::DEFAULT_CONFIG_FILE = "config.json";

    const std::string CLIBase::kClassName("CLIBase");

    CLIBase::CLIBase() : is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_to_cli_string_(false), argument_desc_("Allowed arguments:"), argument_info_()
    {
        // Verify system settings
        uint32_t net_core_rmem_max = Util::getNetCoreRmemMax();
        uint32_t udp_large_recvbuf_size = UdpPktSocket::UDP_LARGE_RCVBUFSIZE;
        if (net_core_rmem_max < udp_large_recvbuf_size)
        {
            std::ostringstream oss;
            oss << "net.core.rmem_max " << std::to_string(net_core_rmem_max) << " < udp_large_recvbuf_size " << std::to_string(udp_large_recvbuf_size) << " -> please run scripts/install_prerequisite.py to set net.core.rmem_max!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        else
        {
            std::ostringstream oss;
            oss << "net.core.rmem_max " << std::to_string(net_core_rmem_max) << " >= udp_large_recvbuf_size " << std::to_string(udp_large_recvbuf_size);
            Util::dumpNormalMsg(kClassName, oss.str());
        }

        clientcnt_ = 0;
        edgecnt_ = 0;
        config_file_ = "";
    }

    CLIBase::~CLIBase() {}

    uint32_t CLIBase::getClientcnt() const
    {
        return clientcnt_;
    }

    uint32_t CLIBase::getEdgecnt() const
    {
        return edgecnt_;
    }

    void CLIBase::parseAndProcessCliParameters(int argc, char **argv)
    {
        std::string main_class_name = Util::getFilenameFromFilepath(argv[0]);

        try
        {
            addCliParameters_(); // Add CLI parameters into argument_desc_
            parseCliParameters_(argc, argv); // Parse CLI parameters based on argument_desc_ to set argument_info_
            setParamAndConfig_(main_class_name); // Set parameters for dynamic configurations and load config file for static configurations
            processCliParameters_(); // Process static/dynamic actions
            dumpCliParameters_(); // Dump CLI parameters
            createRequiredDirectories_(main_class_name); // Create required directories (e.g., client statistics directory and cloud RocksDB directory)
        }
        catch (const std::exception & e)
        {
            std::ostringstream oss;
            oss << "Failed to parse and process CLI parameters:" << e.what();
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }

    std::string CLIBase::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);

            if (clientcnt_ != DEFAULT_CLIENTCNT)
            {
                oss << " --clientcnt " << clientcnt_;
            }
            if (edgecnt_ != DEFAULT_EDGECNT)
            {
                oss << " --edgecnt " << edgecnt_;
            }
            if (config_file_ != DEFAULT_CONFIG_FILE)
            {
                oss << " --config_file " << config_file_;
            }

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void CLIBase::clearIsToCliString()
    {
        is_to_cli_string_ = false;
        return;
    }

    void CLIBase::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            // (1) Create CLI parameter description

            Util::dumpNormalMsg(kClassName, "add CLI parameters into argument_desc_");

            // Common static actions
            argument_desc_.add_options()
                ("help,h", "dump help information")
            ;

            // Common dynamic configurations
            // NOTE: options_description::add_options() will return an instance of options_description_easy_init, whose operator() will create a new option_description and add it to the options_description
            // NOTE: the created option_description will store the argument of (const char* description) into a new string
            // Obsolete: ("debug", "enable debug information"); ("track_event", "track events to break down latencies"), ("multinode", "disable single-node mode (NOT work for simulator)")
            argument_desc_.add_options()
                ("clientcnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_CLIENTCNT), "the total number of clients")
                ("edgecnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_EDGECNT), "the number of edge nodes")
                ("config_file", boost::program_options::value<std::string>()->default_value(DEFAULT_CONFIG_FILE), "config file path of COVERED")
            ;

            // Common dynamic actions
            argument_desc_.add_options()
                ("version,v", "dump version of COVERED")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void CLIBase::parseCliParameters_(int argc, char **argv)
    {
        // (2) Parse CLI parameters (static/dynamic actions and dynamic configurations)

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc_), argument_info_);
        boost::program_options::notify(argument_info_);

        return;
    }

    void CLIBase::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            // (3) Get CLI parameters for common dynamic configurations

            uint32_t clientcnt = argument_info_["clientcnt"].as<uint32_t>();
            uint32_t edgecnt = argument_info_["edgecnt"].as<uint32_t>();
            std::string config_filepath = argument_info_["config_file"].as<std::string>();

            // Store CLI parameters for dynamic configurations
            clientcnt_ = clientcnt;
            edgecnt_ = edgecnt;
            config_file_ = config_filepath;
            verifyIntegrity_();

            // (4) Load config file for static configurations

            Config::loadConfig(config_filepath, main_class_name);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void CLIBase::processCliParameters_()
    {
        // (5) Process CLI parameters

        // (5.1) Process static actions

        if (argument_info_.count("help")) // Dump help information
        {
            std::ostringstream oss;
            oss << argument_desc_;
            Util::dumpNormalMsg(kClassName, oss.str());
            exit(0);
        }

        // (5.2) Process dynamic actions

        if (argument_info_.count("version"))
        {
            std::ostringstream oss;
            oss << "Current version of COVERED: " << Config::getVersion();
            Util::dumpNormalMsg(kClassName, oss.str());
            exit(0);
        }

        return;
    }

    void CLIBase::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            // (6) Dump stored CLI parameters and parsed config information if debug

            Util::dumpDebugMsg(kClassName, Config::toString());

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Client count: " << clientcnt_;
            oss << "Edge count: " << edgecnt_;
            oss << "Config filepath: " << config_file_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void CLIBase::verifyIntegrity_() const
    {
        assert(clientcnt_ > 0);
        assert(edgecnt_ > 0);
        assert(config_file_ != "");

        if (clientcnt_ < edgecnt_)
        {
            std::ostringstream oss;
            oss << "clientcnt " << clientcnt_ << " should >= edgecnt " << edgecnt_ << " for edge-client mapping!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }
}