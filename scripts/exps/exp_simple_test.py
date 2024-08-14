#!/usr/bin/env python3
# simple_example: an example for usage of componenets in exps.utils.

# (1) Import the correpsonding module you need

# from .utils.dataset_loader import *
# from .utils.prototype import *
# from .utils.single_node_prototype import *
from .utils.single_node_simulator import *
# from .utils.total_statistics_loader import *

# (2) Define the settings for the experiment

# Test single-node simulator
simple_example_settings = {
    "clientcnt": 12,
    "edgecnt": 12,
    "keycnt": 1000000,
    "capacity_mb": 1024,
    "cache_name": "covered", # covered, shark+gdsf, shark+lhd
    "workload_name": "facebook"
}

# # Test COVERED's small LRU cache
# simple_example_settings = {
#     "clientcnt": 12,
#     "edgecnt": 12,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "covered", # covered, shark+gdsf, shark+lhd
#     "workload_name": "facebook"
# }

# # Test Akamai's traces with 25% memory
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 18644, # 149149/8 = 18644 (for akamaiweb), 892448/8 = 111556 (for akamaivideo)
#     "cache_name": "covered", # covered, gdsf+, lhd+
#     "workload_name": "akamaiweb" # akamaiweb, akamaivideo
# }

# # Test Zeta workload generator to reproduce public traces w/o geographical information
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "lhd+", # covered, gdsf+, lhd+
#     "workload_name": "zeta_tencentphoto2" # zeta_wikitext, zeta_wikiimage, zeta_tencentphoto1, zeta_tencentphoto2
# }

# # Extremely large cache hit ratio issue of SIEVE+ under wikitext with 50% memory
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1778,
#     "cache_name": "sieve+",
#     "workload_name": "wikitext"
# }

# # Unreasonable results of GDSF+ of WikiImage and WikiText in AliCloud -> due to unreasonable partition of single-node traces for geo-distributed evaluation
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1778, # 3899 (for wikiimage), 1778 (for wikitext)
#     "cache_name": "covered", # gdsf+, covered
#     "workload_name": "wikitext", # wikiimage, wikitext
#     "realnet_option": "load",
#     "realnet_expname": "exp_alicloud_round1_covered_wikitext", # exp_alicloud_round0_gdsf+_wikiimage, exp_alicloud_round0_covered_wikiimage, exp_alicloud_round0_gdsf+_wikitext, exp_alicloud_round0_covered_wikitext, exp_alicloud_round1_gdsf+_wikiimage, exp_alicloud_round1_covered_wikiimage, exp_alicloud_round1_gdsf+_wikitext, exp_alicloud_round1_covered_wikitext
#     "propagation_latency_clientedge_us": 742, # 750 (for round0), 742 (for round1)
#     "propagation_latency_crossedge_us": 12140, # 10650 (for round0), 12140 (for round1)
#     "propagation_latency_edgecloud_us": 29985, # 30000 (for round0), 29985 (for round1)
# }

# # (DONE) Weight tuner update issue of COVERED under wikiimage with 50% memory -> fixed by filtering abnormal cross-edge latency
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 3899,
#     "cache_name": "covered",
#     "workload_name": "wikiimage"
# }

# # (DONE) Invalid/duplicate response issue of LRB+ under 4M dataset with 50% memory -> fixed by msg seqnum
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 4000000,
#     "capacity_mb": 13675,
#     "cache_name": "lrb+",
#     "workload_name": "facebook"
# }

# # (DONE) Invalid network overhead results of single-node caches under Facebook CDN with 1GiB per-edge memory -> fix by disable directory updates for single-node cachces
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "arc", # arc, gdsf, covered
#     "workload_name": "facebook"
# }

# # (DONE) Inconsistent results of CacheLib after disabling directory updates for single-node caches -> reasonable
# # Reason: as CacheLib uses 60s as LRU refresh time threshold for 2Q eviction policy, larger warmup rate after disabling directory updates means smaller warmup time and hence fewer refreshes -> lower cache hit ratio than not disabling directory updates
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "cachelib+", # cachelib, cachelib+
#     "workload_name": "facebook"
# }

# # (DONE) Try real-net dump and then load -> get reasonable numbers in local testbed
# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "gdsf", # covered, gdsf+, gdsf
#     "workload_name": "facebook",
#     "realnet_option": "load", # dump, load
#     "realnet_expname": "simple_gdsf_1m", # simple_covered_1m, simple_gdsf+_1m, simple_gdsf_1m
# }

# (3) Choose one util component to run

# For dataset loader
#dataset_loader_instance = DatasetLoader(**simple_example_settings)
#dataset_loader_instance.run()

# For prototype
# prototype_instance = Prototype(**simple_example_settings)
# prototype_instance.run()

# For single-node prototype
#single_node_prototype_instance = SingleNodePrototype(**simple_example_settings)
#single_node_prototype_instance.run()

# For single-node simulator
single_node_simulator_instance = SingleNodeSimulator(**simple_example_settings)
single_node_simulator_instance.run()

# For total statistics loader
#total_statistics_loader_instance = TotalStatisticsLoader(**simple_example_settings)
#total_statistics_loader_instance.run()