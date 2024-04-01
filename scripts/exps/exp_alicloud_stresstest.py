#!/usr/bin/env python3
# exp_alicloud_stresstest: stresstest phase for performance analysis in AliCloud.

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
    "workload_name": "facebook",
    "propagation_latency_clientedge_us": Common.alicloud_avg_clientedge_latency_us,
    "propagation_latency_crossedge_us": Common.alicloud_avg_crossedge_latency_us,
    "propagation_latency_edgecloud_us": Common.alicloud_avg_edgecloud_latency_us,
    "realnet_option": "load",
    "realnet_expname": "",
}
# NOTE: run lrb, glcache, and segcache at last due to slow warmup issue of lrb (may be caused by model retraining), and memory usage issue of segcache and glcache (may be caused by bugs on segment-level memory management) -> TODO: if no results of the above baselines due to program crashes, please provide more DRAM memory (or swap memory), and run them again with sufficient time (may be in units of hours or days) for warmup and cache stable performance
#cache_names = ["covered", "shark", "bestguess", "arc+", "cachelib+", "fifo+", "frozenhot+", "gdsf+", "lfu+", "lhd+", "s3fifo+", "sieve+", "wtinylfu+", "lrb+", "glcache+", "segcache+"]
cache_names = ["covered", "gdsf+", "lhd+"] # TMPDEBUG
peredge_capacity_list = [1024, 2048, 4096, 8192] # 1G, 2G, 4G, and 8G
workload_peredge_capacity_map = {"facebook": 3479, "wikitext": 1778, "wikiimage": 3899} # per-edge capacity = dataset capacity * 50% / 4 edges (OBSOLETE due to single-node traces)

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_alicloud_stresstest/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run prototype for each cache name
    for tmp_cache_name in cache_names:

        # (1) Run prototype for each per-edge memory capacity
        for tmp_capacity_mb in peredge_capacity_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_capacity_mb)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ per-edge memory capacity {} MiB for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_capacity_mb, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["capacity_mb"] = tmp_capacity_mb
            tmp_exp_settings["cache_name"] = tmp_cache_name
            tmp_exp_settings["realnet_expname"] = "exp_alicloud_round{}_{}_{}".format(tmp_round_index, tmp_cache_name, tmp_capacity_mb)

            # NOTE: you can try the following special cases with 10M warmup requests if you have sufficient memory for SegCache, yet our evaluation results and conclusions should NOT change
            # Special case: for segcache+ under 8 GiB per-edge memory, use 4M instead of 10M requests to warmup to avoid being killed by OS kernel due to using up all memory in cache node, which is acceptable due to the following reasons:
            # (i) The memory issue is caused by the implementation bug (maybe memory leakage) of SegCache itself instead of cooperative caching;
            # (ii) SegCache is actually already warmed up with 4M requests, as from 4M requests until SegCache fails due to memory issue, the cache hit ratio holds stable -> NOT affect our evaluation results;
            # (iii) SegCache is not the best baseline, so slight variation of cache stable performance does not affect our evaluation conclusions.
            if tmp_cache_name == "segcache+" and tmp_capacity_mb == 8192:
                tmp_exp_settings["warmup_reqcnt_scale"] = 4 # 4M warmup reqcnt

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ per-edge memory capacity {} for the current round {}...".format(tmp_cache_name, tmp_capacity_mb, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()
        
        # (2) Run prototype for each workload (OBSOLETE due to single-node traces)
        for tmp_workload, tmp_peredge_capacity in workload_peredge_capacity_map.items():
            tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_workload)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {} for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_workload, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["capacity_mb"] = tmp_peredge_capacity # 50% dataset capacity / 4 edges
            tmp_exp_settings["cache_name"] = tmp_cache_name
            tmp_exp_settings["workload_name"] = tmp_workload
            tmp_exp_settings["realnet_expname"] = "exp_alicloud_round{}_{}_{}".format(tmp_round_index, tmp_cache_name, tmp_workload)

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {} for the current round {}...".format(tmp_cache_name, tmp_workload, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()

# Hint users to check stresstest statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stresstest statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))