#!/usr/bin/env python3
# exp_simulation_intercache_latency: performance evaluation for changing average inter-cache latency experiments by single-node simulation.

from .utils.single_node_simulator import *

# NOTE: NO need to check if current machine is an evaluator machine, as we do NOT really launch multiple components

# Used to hint users for stable statistics on cache performance
log_dirpaths = []

# Get round indexes for the current experiment
round_indexes = range(0, Common.exp_round_number) # [0, ..., exp_round_number-1]
round_indexes = [0]
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
}

largescale_keycnt = 10 * 1000000 # 10M (default 1M is too small for large-scale exps that all methods will achieve nearly full hit ratio)
cache_names = ["covered", "shark+gdsf", "shark+lhd"] # NOTE: just for fast evaluation -> you can add more methods if with time
# cache_names = ["covered", "shark", "bestguess", "magnet", "adaptsize", "arc", "cachelib", "fifo", "frozenhot", "gdsf", "lacache", "lfu", "lhd", "lru", "s3fifo", "sieve", "wtinylfu", "lrb", "glcache", "segcache", "shark+adaptsize", "shark+arc", "shark+cachelib", "shark+fifo", "shark+frozenhot", "shark+gdsf", "shark+lacache", "shark+lfu", "shark+lhd", "shark+s3fifo", "shark+sieve", "shark+wtinylfu", "shark+lrb", "shark+glcache", "shark+segcache"]
# Help me set up the parameter set.
# Uniform distribution: 2000-4000, 2000-8000, 2000-12000
# Poison distribution: lamba = 3000, 6000, 9000 [2000-12000]
# Pareto distribution: k = 1.0, 1.5, 2.0 [2000-12000]
# each group paras contains a distname,lbound, avg, rbound
# if the distribution has extra paras, like poisson, we use "poisson_lamba" 
#  init the para list for me, use a tuple list to store all paras


distribution_params = [
    ("uniform", 2000, 4000),
    ("uniform", 2000, 8000),
    ("uniform", 2000, 12000),
    ("poisson_3000", 2000, 12000),
    ("poisson_6000", 2000, 12000),
    ("poisson_9000", 2000, 12000),
    ("pareto_1.0", 2000, 12000),
    ("pareto_1.5", 2000, 12000),
    ("pareto_2.0", 2000, 12000),
]

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
        for i in range(len(distribution_params)):
            tmp_distname, tmp_lbound_latency, tmp_rbound_latency = distribution_params[i]
            # tmp_avg_latency is meaningless for poisson and pareto, we just set it to the mid value of lbound and rbound
            if tmp_distname.startswith("uniform"):
                tmp_avg_latency = int((tmp_lbound_latency + tmp_rbound_latency) / 2)
            elif tmp_distname.startswith("poisson"):
                tmp_avg_latency = int((tmp_lbound_latency + tmp_rbound_latency) / 2)
            elif tmp_distname.startswith("pareto"):
                tmp_avg_latency = int((tmp_lbound_latency + tmp_rbound_latency) / 2)
            else:
                # error distname
                LogUtil.error(Common.scriptname, "Unknown distribution name: {}".format(tmp_distname))
                continue
            # rewrite the log file path
            tmp_log_filepath = ""
            # "{}/tmp_evaluator_for_{}_{}_avg{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_distname, tmp_avg_latency)
            if tmp_distname.startswith("uniform"):
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_distname, tmp_avg_latency)
            elif tmp_distname.startswith("poisson"):
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_distname)
            elif tmp_distname.startswith("pareto"):
                # tmp_avg_latency = int((tmp_lbound_latency + tmp_rbound_latency) / 2)            
                tmp_log_filepath = "{}/tmp_evaluator_for_{}_{}.out".format(tmp_log_dirpath, tmp_cache_name, tmp_distname)
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
            tmp_exp_settings["propagation_latency_distname"] = tmp_distname
            
            # Launch single-node simulator
            LogUtil.prompt(Common.scriptname, "Run single-node simulator of {} w/ {} inter-cache avg latency for the current round {} under {}...".format(tmp_cache_name, tmp_avg_latency, tmp_round_index, tmp_distname))
            simulator_instance = SingleNodeSimulator(single_node_logfile = tmp_log_filepath, **tmp_exp_settings)
            simulator_instance.run()

# Hint users to check stable statistics of cache peformance in log files
LogUtil.emphasize(Common.scriptname, "Please check cache stable statistics in log files (at the end of each log file) in the following directories:\n{}".format(log_dirpaths))