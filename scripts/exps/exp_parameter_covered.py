#!/usr/bin/env python3
# exp_performance_covered: parameter analysis on different covered settings.

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
    "covered_local_uncached_max_mem_usage_mb": 1,
    "covered_popularity_aggregation_max_mem_usage_mb": 1,
    "covered_popularity_collection_change_ratio": 0.1,
    "covered_topk_edgecnt": 1,
    "covered_peredge_synced_victimcnt": 3
}

# NOTE: NO need to run the default setting for COVERED, which is the same as previous experiment (performance against existing methods)
covered_local_uncached_max_mem_usage_mb_list = [5, 10] # 5MiB and 10MiB
covered_popularity_aggregation_max_mem_usage_mb_list = [5, 10] # 5MiB and 10MiB
#covered_popularity_collection_change_ratio_list = [0.5, 1.0] # 50% and 100%
covered_popularity_collection_change_ratio_list = [0.2, 0.4, 0.8] # 20%, 40%, and 80%
covered_topk_edgecnt_list = [2, 3, 4] # Up to 4 as we use 4 cache nodes by default
covered_peredge_synced_victimcnt_list = [10, 30] # 10 and 30 synced victims each time

# NOTE: due to the same conclusions for all the parameters of COVERED, we ONLY show the results of covered_topk_edgecnt for limited space in paper
test_covered_local_uncached_max_mem_usage_mb = False
test_covered_popularity_aggregation_max_mem_usage_mb = False
test_covered_popularity_collection_change_ratio = True
test_covered_topk_edgecnt = True
test_covered_peredge_synced_victimcnt = False

# Run the experiments with multiple rounds
for tmp_round_index in round_indexes:
    tmp_log_dirpath = "{}/exp_parameter_covered/round{}".format(Common.output_log_dirpath, tmp_round_index)
    log_dirpaths.append(tmp_log_dirpath)

    # Create log dirpath if necessary
    if not os.path.exists(tmp_log_dirpath):
        LogUtil.prompt(Common.scriptname, "Create log dirpath {} for the current round {}...".format(tmp_log_dirpath, tmp_round_index))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_log_dirpath, keep_silent = True)
    
    # (1) Run prototype for each memory for local uncached popularity
    if test_covered_local_uncached_max_mem_usage_mb:
        for tmp_covered_local_uncached_max_mem_usage_mb in covered_local_uncached_max_mem_usage_mb_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_covered_localmem{}.out".format(tmp_log_dirpath, tmp_covered_local_uncached_max_mem_usage_mb)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip COVERED w/ {}MiB memory for local uncached popularity for the current round {}...".format(tmp_log_filepath, tmp_covered_local_uncached_max_mem_usage_mb, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["covered_local_uncached_max_mem_usage_mb"] = tmp_covered_local_uncached_max_mem_usage_mb

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of COVERED w/ {}MiB memory for local uncached popularity for the current round {}...".format(tmp_covered_local_uncached_max_mem_usage_mb, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()
    
    # (2) Run prototype for each memory for global uncached rewards
    if test_covered_popularity_aggregation_max_mem_usage_mb:
        for tmp_covered_popularity_aggregation_max_mem_usage_mb in covered_popularity_aggregation_max_mem_usage_mb_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_covered_globalmem{}.out".format(tmp_log_dirpath, tmp_covered_popularity_aggregation_max_mem_usage_mb)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip COVERED w/ {}MiB memory for global uncached rewards for the current round {}...".format(tmp_log_filepath, tmp_covered_popularity_aggregation_max_mem_usage_mb, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["covered_popularity_aggregation_max_mem_usage_mb"] = tmp_covered_popularity_aggregation_max_mem_usage_mb

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of COVERED w/ {}MiB memory for global uncached rewards for the current round {}...".format(tmp_covered_popularity_aggregation_max_mem_usage_mb, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()

    # (3) Run prototype for each threshold of popularity change ratio
    if test_covered_popularity_collection_change_ratio:
        for tmp_covered_popularity_collection_change_ratio in covered_popularity_collection_change_ratio_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_covered_popchange{}.out".format(tmp_log_dirpath, tmp_covered_popularity_collection_change_ratio)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip COVERED w/ {} threshold of popularity change ratio for the current round {}...".format(tmp_log_filepath, tmp_covered_popularity_collection_change_ratio, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["covered_popularity_collection_change_ratio"] = tmp_covered_popularity_collection_change_ratio

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of COVERED w/ {} threshold of popularity change ratio for the current round {}...".format(tmp_covered_popularity_collection_change_ratio, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()
    
    # (4) Run prototype for each k of top-k popularity for global admission and trade-off-aware cache placement
    if test_covered_topk_edgecnt:
        for tmp_covered_topk_edgecnt in covered_topk_edgecnt_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_covered_topk{}.out".format(tmp_log_dirpath, tmp_covered_topk_edgecnt)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip COVERED w/ top-{} popularity for the current round {}...".format(tmp_log_filepath, tmp_covered_topk_edgecnt, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["covered_topk_edgecnt"] = tmp_covered_topk_edgecnt

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of COVERED w/ top-{} popularity for the current round {}...".format(tmp_covered_topk_edgecnt, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()
    
    # (5) Run prototype for each synced victimcnt
    if test_covered_peredge_synced_victimcnt:
        for tmp_covered_peredge_synced_victimcnt in covered_peredge_synced_victimcnt_list:
            tmp_log_filepath = "{}/tmp_evaluator_for_covered_victimcnt{}.out".format(tmp_log_dirpath, tmp_covered_peredge_synced_victimcnt)
            SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))

            # Check log filepath
            if os.path.exists(tmp_log_filepath):
                LogUtil.prompt(Common.scriptname, "Log filepath {} already exists, skip COVERED w/ {} synced victims for the current round {}...".format(tmp_log_filepath, tmp_covered_peredge_synced_victimcnt, tmp_round_index))
                continue

            # NOTE: Log filepath MUST NOT exist here

            # Prepare settings for the current cache name
            tmp_exp_settings = exp_default_settings.copy()
            tmp_exp_settings["covered_peredge_synced_victimcnt"] = tmp_covered_peredge_synced_victimcnt

            # Launch prototype
            LogUtil.prompt(Common.scriptname, "Run prototype of COVERED w/ {} synced victims for the current round {}...".format(tmp_covered_peredge_synced_victimcnt, tmp_round_index))
            prototype_instance = Prototype(evaluator_logfile = tmp_log_filepath, **tmp_exp_settings)
            prototype_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))