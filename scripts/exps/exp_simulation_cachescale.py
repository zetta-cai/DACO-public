#!/usr/bin/env python3
# exp_simulation_cachescale: performance evaluation for large-scale experiments by single-node simulation.

from .utils.single_node_simulator import *

# NOTE: NO need to check if current machine is an evaluator machine, as we do NOT really launch multiple components

# Used to hint users for stable statistics on cache performance
log_dirpaths = []

# Get round indexes for the current experiment
# round_indexes = range(0, Common.exp_round_number) # [0, ..., exp_round_number-1]
round_indexes = [0] # TMPDEBUG24

# Prepare settings for current experiment
exp_default_settings = {
    "clientcnt": 12,
    "edgecnt": 12,
    "keycnt": 1000000,
    "capacity_mb": 1024,
    "cache_name": "covered",
    "workload_name": "facebook"
}

cache_names = ["covered", "shark+gdsf", "shark+lhd", "shark", "gdsf", "lhd", "magnet", "bestguess"]
# edgecnt_list = [32, 64, 128, 256, 512, 1024]
edgecnt_list = [12] # TMPDEBUG24

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_simulation_cachescale/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run single-node simulator for each cache name
    for tmp_cache_name in cache_names:

        # Run single-node simulator for each cache scale
        for tmp_edgecnt in edgecnt_list:

            tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_edgecnt)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {} edgecnt for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_edgecnt, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["clientcnt"] = tmp_edgecnt
            tmp_exp_settings["edgecnt"] = tmp_edgecnt
            tmp_exp_settings["cache_name"] = tmp_cache_name

            # Launch single-node simulator
            LogUtil.prompt(Common.scriptname, "Run single-node simulator of {} w/ {} edgecnt for the current round {}...".format(tmp_cache_name, tmp_edgecnt, tmp_round_index))
            simulator_instance = SingleNodeSimulator(single_node_logfile = tmp_log_filepath, **tmp_exp_settings)
            simulator_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))