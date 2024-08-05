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
    "propagation_latency_clientedge_us": 500,
    "propagation_latency_crossedge_us": 5000,
    "propagation_latency_edgecloud_us": 50000
}
# NOTE: run lrb, glcache, and segcache at last due to slow warmup issue of lrb (may be caused by model retraining), and memory usage issue of segcache and glcache (may be caused by bugs on segment-level memory management) -> TODO: if no results of the above baselines due to program crashes, please provide more DRAM memory (or swap memory), and run them again with sufficient time (may be in units of hours or days) for warmup and cache stable performance
cache_names = ["covered", "shark", "bestguess", "arc+", "cachelib+", "fifo+", "frozenhot+", "gdsf+", "lfu+", "lhd+", "s3fifo+", "sieve+", "wtinylfu+", "lrb+", "glcache+", "segcache+"]
# NOTE: NO need to run 500-5000-50000, which is the same as previous experiments (performance against existing and extended methods)
clientedge_latency_list = [100, 2500] # 0.1ms and 2.5ms of 1/2 RTT
crossedge_latency_list = [1000, 25000] # 1ms and 25ms of 1/2 RTT
edgecloud_latency_list = [10000, 250000] # 10ms and 250ms of 1/2 RTT

# NOTE: due to the same conclusions for all the latency settings, we ONLY show the results of cross-cache latency for limited space in paper
test_clientedge_latency = True
test_crossedge_latency = True
test_edgecloud_latency = True

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

        # (1) Run prototype for each client-edge latency
        if test_clientedge_latency:
            for tmp_clientedge_latency in clientedge_latency_list:
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_clientedge{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_clientedge_latency)
                SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

                # Check log filepath
                if os.path.exists(tmp_log_filepath):
                    LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {}us client-edge latency for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_clientedge_latency, tmp_round_index))
                    continue

                # NOTE: Log filepath MUST NOT exist here

                # Prepare settings for the current cache name
                tmp_exp_settings = exp_default_settings.copy()
                tmp_exp_settings["cache_name"] = tmp_cache_name
                tmp_exp_settings["propagation_latency_clientedge_us"] = tmp_clientedge_latency

                # Launch prototype
                LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {}us client-edge latency for the current round {}...".format(tmp_cache_name, tmp_clientedge_latency, tmp_round_index))
                prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
                prototype_instance.run()
        
        # (2) Run prototype for each cross-edge latency
        if test_crossedge_latency:
            for tmp_crossedge_latency in crossedge_latency_list:
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_crossedge{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_crossedge_latency)
                SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

                # Check log filepath
                if os.path.exists(tmp_log_filepath):
                    LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {}us cross-edge latency for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_crossedge_latency, tmp_round_index))
                    continue

                # NOTE: Log filepath MUST NOT exist here

                # Prepare settings for the current cache name
                tmp_exp_settings = exp_default_settings.copy()
                tmp_exp_settings["cache_name"] = tmp_cache_name
                tmp_exp_settings["propagation_latency_crossedge_us"] = tmp_crossedge_latency

                # Launch prototype
                LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {}us cross-edge latency for the current round {}...".format(tmp_cache_name, tmp_crossedge_latency, tmp_round_index))
                prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
                prototype_instance.run()
        
        # (3) Run prototype for each edge-cloud latency
        if test_edgecloud_latency:
            for tmp_edgecloud_latency in edgecloud_latency_list:
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_edgecloud{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_edgecloud_latency)
                SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

                # Check log filepath
                if os.path.exists(tmp_log_filepath):
                    LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {}us edge-cloud latency for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_edgecloud_latency, tmp_round_index))
                    continue

                # NOTE: Log filepath MUST NOT exist here

                # Prepare settings for the current cache name
                tmp_exp_settings = exp_default_settings.copy()
                tmp_exp_settings["cache_name"] = tmp_cache_name
                tmp_exp_settings["propagation_latency_edgecloud_us"] = tmp_edgecloud_latency

                # Launch prototype
                LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {}us edge-cloud latency for the current round {}...".format(tmp_cache_name, tmp_edgecloud_latency, tmp_round_index))
                prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
                prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))