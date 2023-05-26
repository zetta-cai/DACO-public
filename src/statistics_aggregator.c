#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <time.h> // struct timespec
#include <unistd.h> // usleep

#include "common/cli.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "statistics/client_statistics_tracker.h"
#include "statistics/total_statistics_tracker.h"

int main(int argc, char **argv) {
    std::string main_class_name = "statistics_aggregator";

    // (1) Parse and process CLI parameters (set configurations in Config and Param)
    CLI::parseAndProcessCliParameters(main_class_name);

    // TODO: === END HERE ===

    int pthread_returncode;

    // (1) TODO: Launch clientcnt loaders to load per-client statistics

    // (2) TODO: Aggregate and dump total statistics

    // (3) TODO: Wait for clientcnt loaders

    // (4) TODO: Release variables in heap

    return 0;
}