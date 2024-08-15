#!/usr/bin/env python3
# exp_performance_workloads: performance evaluation for workloads methods.

from .utils.prototype import *

# Check if current machine is an evaluator machine
evaluator_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "evaluator_machine_index")
if Common.cur_machine_idx != evaluator_machine_idx:
    LogUtil.die(Common.scriptname, "This script is only allowed to run on the evaluator machine")

# Used to hint users for stable statistics on cache performance
log_dirpaths = []

# Get round indexes for the current experiment
round_indexes = range(0, Common.exp_round_number) # [0, ..., exp_round_number-1]

# Prepare settings for current experiment
exp_default_settings = {
    "clientcnt": 12,
    "edgecnt": 12,
    "keycnt": 1000000,
    "capacity_mb": 1024,
    "cache_name": "covered",
    "workload_name": "facebook"
}
# NOTE: run lrb, glcache, and segcache at last due to slow warmup issue of lrb (may be caused by model retraining), and memory usage issue of segcache and glcache (may be caused by bugs on segment-level memory management) -> TODO: if no results of the above baselines due to program crashes, please provide more DRAM memory (or swap memory), and run them again with sufficient time (may be in units of hours or days) for warmup and cache stable performance
# cache_names = ["covered", "shark", "bestguess", "shark+arc", "shark+cachelib", "shark+fifo", "shark+frozenhot", "shark+gdsf", "shark+lfu", "shark+lhd", "shark+s3fifo", "shark+sieve", "shark+wtinylfu", "shark+lrb", "shark+glcache", "shark+segcache"]
cache_names = ["covered", "shark+gdsf", "shark+lhd"] # NOTE: just for fast evaluation -> you can use the above cache list if with time
# NOTE: NO need to run facebook, which has been done in previous experiments (performance against existing methods and extended methods)
# workloads = ["zeta_wikiimage", "zeta_wikitext", "zeta_tencentphoto1", "zeta_tencentphoto2"]
workloads = ["zipf_wikiimage", "zipf_wikitext", "zipf_tencentphoto1", "zipf_tencentphoto2"] # Use power-law Zipf distribution for consistency with zipf_facebook

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_performance_workloads/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run prototype for each cache name
    for tmp_cache_name in cache_names:

        # Run prototype for each workload
        for tmp_workload in workloads:
            tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_workload)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ workload {} for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_workload, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["cache_name"] = tmp_cache_name
            tmp_exp_settings["workload_name"] = tmp_workload

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ workload {} for the current round {}...".format(tmp_cache_name, tmp_workload, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))