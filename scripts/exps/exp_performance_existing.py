#!/usr/bin/env python3
# exp_performance_existing: performance evaluation for existing methods.

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
    "clientcnt": 4,
    "edgecnt": 4,
    "keycnt": 1000000,
    "capacity_mb": 1024,
    "cache_name": "covered",
    "workload_name": "facebook"
}
#cache_names = ["covered", "shark", "bestguess", "arc", "cachelib", "fifo", "frozenhot", "glcache", "lrb", "gdsf", "lfu", "lhd", "lru", "s3fifo", "segcache", "sieve", "wtinylfu"]
cache_names = ["glcache"] # TMPDEBUG24

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_performance_existing/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run prototype for each cache name
    for tmp_cache_name in cache_names:
        tmp_log_filepath = "{}/tmp_evaluator_for_{}.out".format(tmp_log_dirpath, tmp_cache_name)
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

        # Check log filepath
        if os.path.exists(tmp_log_filepath):
            LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_round_index))
            continue

        # NOTE: Log filepath MUST NOT exist here

        # Prepare settings for the current cache name
        tmp_exp_settings = exp_default_settings
        tmp_exp_settings["cache_name"] = tmp_cache_name

        # Launch prototype
        LogUtil.prompt(Common.scriptname, "Run prototype of {} for the current round {}...".format(tmp_cache_name, tmp_round_index))
        prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
        prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files in the following directories:\n{}".format(log_dirpaths))