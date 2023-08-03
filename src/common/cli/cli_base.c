#include "common/cli/cli_base.h"

#include "common/config.h"
#include "common/param/common_param.h"
#include "common/util.h"

namespace covered
{
    const std::string CLIBase::kClassName("CLIBase");

    CLIBase::CLIBase() : is_add_cli_parameters_(false), is_set_param_and_config_(false), argument_desc_("Allowed arguments:"), argument_info_()
    {
    }

    CLIBase::~CLIBase() {}

    void CLIBase::parseAndProcessCliParameters(int argc, char **argv)
    {
        std::string main_class_name = Util::getFilenameFromFileath(argv[0]);

        addCliParameters_(); // Add CLI parameters into argument_desc_
        parseCliParameters_(argc, argv); // Parse CLI parameters based on argument_desc_ to set argument_info_
        setParamAndConfig_(main_class_name); // Set parameters for dynamic configurations and load config file for static configurations
        processCliParameters_(); // Process static/dynamic actions
        createRequiredDirectories_(main_class_name); // Create required directories (e.g., client statistics directory and cloud RocksDB directory)

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
            argument_desc_.add_options()
                ("config_file", boost::program_options::value<std::string>()->default_value("config.json"), "config file path of COVERED")
                ("debug", "enable debug information")
                ("multinode", "disable single-node mode (NOT work for simulator)")
                ("track_event", "track events to break down latencies")
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

            bool is_single_node = true; // Enable single-node mode by default
            if (argument_info_.count("multinode"))
            {
                if (main_class_name == CommonParam::SIMULATOR_MAIN_NAME)
                {
                    std::ostringstream oss;
                    oss << "--multinode does not work for " << main_class_name << " -> still enable single-node mode!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
                else
                {
                    is_single_node = false;
                }
            }
            std::string config_filepath = argument_info_["config_file"].as<std::string>();
            bool is_debug = false;
            if (argument_info_.count("debug"))
            {
                is_debug = true;
            }
            bool track_event = false;
            if (argument_info_.count("track_event"))
            {
                track_event = true;
            }

            // Store CLI parameters for common dynamic configurations and mark CommonParam as valid
            CommonParam::setParameters(main_class_name, is_single_node, config_filepath, is_debug, track_event);

            // (4) Load config file for static configurations

            Config::loadConfig();

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

        // (6) Dump stored CLI parameters and parsed config information if debug

        Util::dumpDebugMsg(kClassName, Config::toString());
        Util::dumpDebugMsg(kClassName, CommonParam::toString());

        return;
    }
}