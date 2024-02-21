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
#include "cli/cliutil_cli.h"
#include "common/config.h"
#include "common/util.h"
#include "statistics/total_statistics_tracker.h"

int main(int argc, char **argv) {

    // NOTE: used by exp scripts to verify whether evaluation has been done -> MUST be the same as scripts/exps/utils/cliutil.py
    const std::string CLIENT_PREFIX_STRING = "Client:";
    const std::string EDGE_PREFIX_STRING = "Edge:";
    const std::string CLOUD_PREFIX_STRING = "Cloud:";
    const std::string EVALUATOR_PREFIX_STRING = "Evaluator:";
    const std::string SIMULATOR_PREFIX_STRING = "Simulator:";
    const std::string DATASET_LOADER_PREFIX_STRING = "DatasetLoader:";
    const std::string TRACE_PREPROCESSOR_PREFIX_STRING = "TracePreprocessor:";

    // (1) Parse and process CLI parameters
    // NOTE: use CliutilCLI to parse CLI parameters for all possible components
    covered::CliutilCLI cliutil_cli(argc, argv);

    // (2) Print CLI string for different components

    // Get CLI string of different componenets
    std::string client_cli_string = ((covered::ClientCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string edge_cli_string = ((covered::EdgeCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string cloud_cli_string = ((covered::CloudCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string evaluator_cli_string = ((covered::EvaluatorCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string simulator_cli_string = ((covered::SimulatorCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string dataset_loader_cli_string = ((covered::DatasetLoaderCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    std::string trace_preprocessor_cli_string = ((covered::TracePreprocessorCLI*)&cliutil_cli)->toCliString();
    cliutil_cli.clearIsToCliString();
    
    // Print CLI string of different components
    std::ostringstream oss;
    oss << CLIENT_PREFIX_STRING << client_cli_string << std::endl;
    oss << EDGE_PREFIX_STRING << edge_cli_string << std::endl;
    oss << CLOUD_PREFIX_STRING << cloud_cli_string << std::endl;
    oss << EVALUATOR_PREFIX_STRING << evaluator_cli_string << std::endl;
    oss << SIMULATOR_PREFIX_STRING << simulator_cli_string << std::endl;
    oss << DATASET_LOADER_PREFIX_STRING << dataset_loader_cli_string << std::endl;
    oss << TRACE_PREPROCESSOR_PREFIX_STRING << trace_preprocessor_cli_string << std::endl;
    std::cout << oss.str();

    return 0;
}