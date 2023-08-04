/*
 * Load per-slot/stable total aggregated statistics dumped by evaluator and print.
 * 
 * By Siyuan Sheng (2023.07.27).
 */

#include <sstream>

#include "common/cli/evaluator_cli.h"
#include "common/util.h"
#include "statistics/total_statistics_tracker.h"

int main(int argc, char **argv) {
    // (1) Parse and process CLI parameters and store them into EvaluatorParam
    covered::EvaluatorCLI evaluator_cli(argc, argv);

    const std::string main_class_name = evaluator_cli.getMainClassName();

    // (2) Load total aggregated statistics
    std::string total_statistics_filepath = covered::Util::getEvaluatorStatisticsFilepath();
    std::ostringstream oss;
    oss << "load per-slot/stable total aggregated statistics from " << total_statistics_filepath;
    covered::Util::dumpNormalMsg(main_class_name, oss.str());
    covered::TotalStatisticsTracker total_statistics_tracker(total_statistics_filepath);

    // (3) Print total aggregated statistics
    oss.clear();
    oss.str("");
    oss << total_statistics_tracker.toString();
    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    return 0;
}