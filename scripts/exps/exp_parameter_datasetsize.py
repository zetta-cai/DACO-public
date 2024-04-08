#!/usr/bin/env python3
# exp_performance_datasetsize: parameter analysis on different dataset sizes (# of unique objects).

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
# NOTE: run lrb, glcache, and segcache at last due to slow warmup issue of lrb (may be caused by model retraining), and memory usage issue of segcache and glcache (may be caused by bugs on segment-level memory management) -> TODO: if no results of the above baselines due to program crashes, please provide more DRAM memory (or swap memory), and run them again with sufficient time (may be in units of hours or days) for warmup and cache stable performance
cache_names = ["covered", "shark", "bestguess", "arc+", "cachelib+", "fifo+", "frozenhot+", "gdsf+", "lfu+", "lhd+", "s3fifo+", "sieve+", "wtinylfu+", "lrb+", "glcache+", "segcache+"]
# # NOTE: NO need to run 1M, which is the same as previous experiment (performance against different workloads)
# keycnt_peredge_capacity_map = {"2000000": 6887, "4000000": 13675} # 2M and 4M dataset size; per-edge capacity = dataset capacity * 50% / 4 edges
# NOTE: NO need to run 1M, which is the same as previous experiment (performance on existing methods)
keycnt_peredge_capacity_map = {"2000000": 1024, "4000000": 1024} # 2M and 4M dataset size; per-edge capacity = 1 GiB by default

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_parameter_datasetsize/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # Run prototype for each cache name
    for tmp_cache_name in cache_names:

        # Run prototype for each dataset size
        for tmp_keycnt, tmp_peredge_capacity in keycnt_peredge_capacity_map.items():
            tmp_keycnt = int(tmp_keycnt) # Conert string to integer
            tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_keycnt, tmp_peredge_capacity)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip {} w/ {} dataset size ({} GiB cache space) for the current round {}...".format(tmp_log_filepath, tmp_cache_name, tmp_keycnt, tmp_peredge_capacity, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["keycnt"] = tmp_keycnt
            tmp_exp_settings["capacity_mb"] = tmp_peredge_capacity # 50% dataset capacity / 4 edges
            tmp_exp_settings["cache_name"] = tmp_cache_name

            # NOTE: you can try the following special cases with 20M/40M warmup requests if you have sufficient memory for SegCache and LRB, yet our evaluation results and conclusions should NOT change
            # Special case 1: for LRB+ under 4M dataset, we can use 8M warmup requests instead of 40M ones to avoid too slow warmup due to model retraining and large memory cost, which is acceptable due to the following reasons:
            # (i) LRB+ has already been warmed up at 8M requests instead of until 40M requests, which has no difference for cache stable performance of LRB+ -> NOT affect evaluation results;
            # (ii) LRB+ is not the best baseline -> slight variation of cache stable performance does not affect our evaluation conclusions.
            # Special case 2: for LRB+ under 2M dataset, use 10M warmup requests instead of 20M to save evaluation time cost (acceptable due to the same reasons as special case 1)
            # Special case 3 and 4: for SegCache+ under 2M/4M dataset, use 6M/8M warmup requests instead of 20M/40M ones to avoid too large memory cost and too slow warmup due to segment-level memory management bugs, which is acceptable due to the same reasons as special case in scripts/exps/exp_parameter_memory.py.
            if tmp_cache_name == "lrb+" and tmp_keycnt == 4000000:
                tmp_exp_settings["warmup_reqcnt_scale"] = 2 # 8M warmup reqcnt
            elif tmp_cache_name == "lrb+" and tmp_keycnt == 2000000:
                tmp_exp_settings["warmup_reqcnt_scale"] = 5 # 10M warmup reqcnt
            elif tmp_cache_name == "segcache+" and tmp_keycnt == 2000000:
                tmp_exp_settings["warmup_reqcnt_scale"] = 3 # 6M warmup reqcnt
            elif tmp_cache_name == "segcache+" and tmp_keycnt == 4000000:
                tmp_exp_settings["warmup_reqcnt_scale"] = 2 # 8M warmup reqcnt

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of {} w/ {} dataset size ({} GiB cache space) for the current round {}...".format(tmp_cache_name, tmp_keycnt, tmp_peredge_capacity, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))