#!/usr/bin/env python3
# exp_performance_latency: parameter analysis on different latency settings.

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
    "workload_name": "facebook",
    "propagation_latency_clientedge_lbound_us": 500,
    "propagation_latency_clientedge_avg_us": 1000, # 1ms
    "propagation_latency_clientedge_rbound_us": 1500,
    "propagation_latency_crossedge_lbound_us": 1500,
    "propagation_latency_crossedge_avg_us": 3000, # 3ms
    "propagation_latency_crossedge_rbound_us": 4500,
    "propagation_latency_edgecloud_lbound_us": 6500,
    "propagation_latency_edgecloud_avg_us": 13000, # 13ms
    "propagation_latency_edgecloud_rbound_us": 19500
}
# NOTE: run lrb, glcache, and segcache at last due to slow warmup issue of lrb (may be caused by model retraining), and memory usage issue of segcache and glcache (may be caused by bugs on segment-level memory management) -> TODO: if no results of the above baselines due to program crashes, please provide more DRAM memory (or swap memory), and run them again with sufficient time (may be in units of hours or days) for warmup and cache stable performance
# cache_names = ["covered", "shark", "bestguess", "shark+adaptsize", "shark+arc", "shark+cachelib", "shark+fifo", "shark+frozenhot", "shark+gdsf", "shark+lfu", "shark+lhd", "shark+s3fifo", "shark+sieve", "shark+wtinylfu", "shark+lrb", "shark+glcache", "shark+segcache"]
cache_names = ["covered", "shark+gdsf", "shark+lhd"] # NOTE: just for fast evaluation -> you can use the above cache list if with time
# NOTE: NO need to run default WAN delay settings, which is the same as previous experiments (performance against existing and extended methods)
large_wan_name = "large"
large_clientedge_lbound = 5000
large_clientedge_avg = 7500
large_clientedge_rbound = 10000
large_crossedge_lbound = 20000
large_crossedge_avg = 35000
large_crossedge_rbound = 50000
large_edgecloud_lbound = 100000
large_edgecloud_avg = 150000
large_edgecloud_rbound = 200000

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_parameter_latency/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run prototype for each cache name
    for tmp_cache_name in cache_names:

        # (1) Run prototype for large WAN delay setting
        tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, large_wan_name)
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

        # Check log filepath
        if os.path.exists(tmp_log_filepath):
            LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {} WAN delay setting for the current round {}...".format(tmp_log_filepath, tmp_cache_name, large_wan_name, tmp_round_index))
            continue

        # NOTE: Log filepath MUST NOT exist here

        # Prepare settings for the current cache name
        tmp_exp_settings = exp_default_settings.copy()
        tmp_exp_settings["cache_name"] = tmp_cache_name
        tmp_exp_settings["propagation_latency_clientedge_lbound_us"] = large_clientedge_lbound
        tmp_exp_settings["propagation_latency_clientedge_avg_us"] = large_clientedge_avg
        tmp_exp_settings["propagation_latency_clientedge_rbound_us"] = large_clientedge_rbound
        tmp_exp_settings["propagation_latency_crossedge_lbound_us"] = large_crossedge_lbound
        tmp_exp_settings["propagation_latency_crossedge_avg_us"] = large_crossedge_avg
        tmp_exp_settings["propagation_latency_crossedge_rbound_us"] = large_crossedge_rbound
        tmp_exp_settings["propagation_latency_edgecloud_lbound_us"] = large_edgecloud_lbound
        tmp_exp_settings["propagation_latency_edgecloud_avg_us"] = large_edgecloud_avg
        tmp_exp_settings["propagation_latency_edgecloud_rbound_us"] = large_edgecloud_rbound

        # Launch prototype
        LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {} WAN delay setting client-edge latency for the current round {}...".format(tmp_cache_name, large_wan_name, tmp_round_index))
        prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
        prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))