/*
 * Help to generate CLI string for python exp scripts.
 * 
 * By Siyuan Sheng (2023.12.30).
 */

#include <sstream>

#include "cli/client_cli.h"
#include "cli/edge_cli.h"
#include "cli/cloud_cli.h"
#include "cli/evaluator_cli.h"
#include "cli/simulator_cli.h"
#include "cli/dataset_loader_cli.h"
#include "common/config.h"
#include "common/util.h"
#include "statistics/total_statistics_tracker.h"

std::string getUsageStr(const std::vector<std::string>& supported_component_names)
{
    std::ostringstream oss;
    oss << "Usage: ./cliutil component_name <CLI parameters>" << std::endl;
    oss << "Supported component_name:";
    for (uint32_t i = 0; i < supported_component_names.size(); i++)
    {
        oss << " " << supported_component_names[i];
    }
    oss << std::endl;
    oss << "Example: ./cliutil simulator --clientcnt 2 --edgecnt 2";

    return oss.str();
}

int main(int argc, char **argv) {
    // // (0) Get component name first
    // std::vector<std::string> supported_component_names;
    // supported_component_names.push_back(covered::Util::CLIENT_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::EDGE_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::CLOUD_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::EVALUATOR_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::SIMULATOR_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::DATASET_LOADER_MAIN_NAME);
    // supported_component_names.push_back(covered::Util::TOTAL_STATISTICS_LOADER_MAIN_NAME);

    // // Check argc
    // if (argc < 2)
    // {
    //     std::ostringstream oss;
    //     oss << "[ERROR] invalid argc: " << argc << std::endl;
    //     oss << getUsageStr(supported_component_names) << std::endl;
    //     return 1;
    // }
    
    // // Check component name
    // std::string tmp_component_name = argv[1];
    // bool is_valid_component_name = false;
    // for (uint32_t i = 0; i < supported_component_names.size(); i++)
    // {
    //     if (tmp_component_name == supported_component_names[i])
    //     {
    //         is_valid_component_name = true;
    //         break;
    //     }
    // }
    // if (!is_valid_component_name)
    // {
    //     std::ostringstream oss;
    //     oss << "[ERROR] invalid component_name: " << tmp_component_name << std::endl;
    //     oss << getUsageStr(supported_component_names) << std::endl;
    //     return 1;
    // }

    // // Prepare new argc and argv for CLI to remove the second CLI parameter of component_name
    // int new_argc = argc - 1;
    // char* new_argv[new_argc];
    // for (int i = 0; i < new_argc; i++)
    // {
    //     if (i == 0)
    //     {
    //         new_argv[i] = argv[i];
    //     }
    //     else
    //     {
    //         new_argv[i] = argv[i + 1];
    //     }
    // }

    // (1) Parse and process CLI parameters
    // NOTE: use SimulatorCLI to parse CLI parameters for all possible components
    covered::EvaluatorCLI simulator_cli(argc, argv);

    std::cout << "HERE" << std::endl; // TMPDEBUG231230

    // (2) Print CLI string for different components

    // Get CLI string of different componenets
    std::string client_cli_string = ((covered::ClientCLI*)&simulator_cli)->toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "client" << std::endl; // TMPDEBUG231230
    std::string edge_cli_string = ((covered::EdgeCLI*)&simulator_cli)->toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "edge" << std::endl; // TMPDEBUG231230
    std::string cloud_cli_string = ((covered::CloudCLI*)&simulator_cli)->toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "cloud" << std::endl; // TMPDEBUG231230
    std::string evaluator_cli_string = ((covered::EvaluatorCLI*)&simulator_cli)->toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "evaluator" << std::endl; // TMPDEBUG231230
    std::string simulator_cli_string = simulator_cli.toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "simulator" << std::endl; // TMPDEBUG231230
    std::string dataset_loader_cli_string = ((covered::DatasetLoaderCLI*)&simulator_cli)->toCliString();
    simulator_cli.clearIsToCliString();
    std::cout << "dataset_loader" << std::endl; // TMPDEBUG231230

    // NOTE: used by exp scripts to verify whether evaluation has been done -> MUST be the same as scripts/exps/utils/cliutil.py
    const std::string CLIENT_PREFIX_STRING = "Client:";
    const std::string EDGE_PREFIX_STRING = "Edge:";
    const std::string CLOUD_PREFIX_STRING = "Cloud:";
    const std::string EVALUATOR_PREFIX_STRING = "Evaluator:";
    const std::string SIMULATOR_PREFIX_STRING = "Simulator:";
    const std::string DATASET_LOADER_PREFIX_STRING = "DatasetLoader:";
    
    // Print CLI string of different components
    std::ostringstream oss;
    oss << CLIENT_PREFIX_STRING << client_cli_string << std::endl;
    oss << EDGE_PREFIX_STRING << edge_cli_string << std::endl;
    oss << CLOUD_PREFIX_STRING << cloud_cli_string << std::endl;
    oss << EVALUATOR_PREFIX_STRING << evaluator_cli_string << std::endl;
    oss << SIMULATOR_PREFIX_STRING << simulator_cli_string << std::endl;
    oss << DATASET_LOADER_PREFIX_STRING << dataset_loader_cli_string << std::endl;
    std::cout << oss.str();

    return 0;
}