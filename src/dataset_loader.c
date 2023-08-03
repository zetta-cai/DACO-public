/*
 * Load dataset of a given workload into the current cloud node offline.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#include <sstream>

#include "common/cli/dataset_loader_cli.h"
#include "common/param/common_param.h"
#include "common/util.h"

int main(int argc, char **argv) {
    // (1) Parse and process CLI parameters and store them into EvaluatorParam
    covered::DatasetLoaderCLI dataset_loader_cli(argc, argv);

    const std::string main_class_name = covered::CommonParam::getMainClassName();

    // (2) Create the corresponding RocksDB KVS
    // TODO

    // (3) Launch dataset loaders to store dataset into RocksDB during loading phase
    // TODO

    return 0;
}