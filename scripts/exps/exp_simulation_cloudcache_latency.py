#!/usr/bin/env python3
# exp_simulation_intercache_latency: performance evaluation for changing average inter-cache latency experiments by single-node simulation.

from .utils.single_node_simulator import *

# NOTE: NO need to check if current machine is an evaluator machine, as we do NOT really launch multiple components

# Used to hint users for stable statistics on cache performance
log_dirpaths = []

# Get round indexes for the current experiment
round_indexes = range(2, Common.exp_round_number) # [0, ..., exp_round_number-1]
round_indexes = [0,2,3,4,]
# Prepare settings for current experiment
exp_default_settings = {
    "clientcnt": 128, # TODO
    "edgecnt": 128, # TODO
    "keycnt": 1000000,
    "capacity_mb": 1024,
    "cache_name": "covered",
    "workload_name": "facebook",
    "simulator_randomness": 0,
    "propagation_latency_distname": "uniform",
    "propagation_latency_crossedge_lbound_us": 1500,
    "propagation_latency_crossedge_avg_us": 3000,
    "propagation_latency_crossedge_rbound_us": 4500,
    "propagation_latency_edgecloud_lbound_us": 6500,
    "propagation_latency_edgecloud_avg_us": 13000, # 13ms
    "propagation_latency_edgecloud_rbound_us": 19500
}

largescale_keycnt = 10 * 1000000 # 10M (default 1M is too small for large-scale exps that all methods will achieve nearly full hit ratio)
cache_names = ["covered", "shark+gdsf", "shark+lhd"] # NOTE: just for fast evaluation -> you can add more methods if with time
# cache_names = ["covered", "shark", "bestguess", "magnet", "adaptsize", "arc", "cachelib", "fifo", "frozenhot", "gdsf", "lacache", "lfu", "lhd", "lru", "s3fifo", "sieve", "wtinylfu", "lrb", "glcache", "segcache", "shark+adaptsize", "shark+arc", "shark+cachelib", "shark+fifo", "shark+frozenhot", "shark+gdsf", "shark+lacache", "shark+lfu", "shark+lhd", "shark+s3fifo", "shark+sieve", "shark+wtinylfu", "shark+lrb", "shark+glcache", "shark+segcache"]
# uniform params setting
# avg 3000 6000 9000 +-

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_simulation_intercache_latency/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run single-node simulator for each cache name
    for tmp_cache_name in cache_names:

        # Run single-node simulator for each cache scale
        for i in range(len(intercache_avg_latency_list)):
            for j in range(len(cloudcache_lbound_latency_list)):
                tmp_avg_edgecloud_latency = cloudcache_lbound_latency_list[j] 
                tmp_lbound_edgecloud_latency = tmp_avg_edgecloud_latency - 3000 # 3ms
                tmp_rbound_edgecloud_latency = tmp_avg_edgecloud_latency + 3000 # 3ms
                
                tmp_lbound_latency = intercache_lbound_latency_list[i]
                tmp_avg_latency = intercache_avg_latency_list[i]
                tmp_rbound_latency = intercache_rbound_latency_list[i]

                tmp_log_filepath = "{}/tmp_evaluator_for_{}_avg{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_avg_edgecloud_latency)
                SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

                # Check log filepath
                if os.path.exists(tmp_log_filepath):
                    LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {} inter-cache avg latency for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_avg_latency, tmp_round_index))
                    continue

                # NOTE: Log filepath MUST NOT exist here

                # Prepare settings for the current cache name
                tmp_exp_settings = exp_default_settings.copy()
                tmp_exp_settings["keycnt"] = largescale_keycnt
                tmp_exp_settings["cache_name"] = tmp_cache_name
                tmp_exp_settings["simulator_randomness"] = tmp_round_index
                tmp_exp_settings["propagation_latency_crossedge_lbound_us"] = tmp_lbound_latency
                tmp_exp_settings["propagation_latency_crossedge_avg_us"] = tmp_avg_latency
                tmp_exp_settings["propagation_latency_crossedge_rbound_us"] = tmp_rbound_latency
                tmp_exp_settings["propagation_latency_edgecloud_lbound_us"] = tmp_lbound_edgecloud_latency
                tmp_exp_settings["propagation_latency_edgecloud_avg_us"] = tmp_avg_edgecloud_latency
                tmp_exp_settings["propagation_latency_edgecloud_rbound_us"] = tmp_rbound_edgecloud_latency

                # Launch single-node simulator
                LogUtil.prompt(Common.scriptname, "Run single-node simulator of {} w/ {} inter-cache avg latency for the current round {}...".format(tmp_cache_name, tmp_avg_latency, tmp_round_index))
                simulator_instance = SingleNodeSimulator(single_node_logfile = tmp_log_filepath, **tmp_exp_settings)
                simulator_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))