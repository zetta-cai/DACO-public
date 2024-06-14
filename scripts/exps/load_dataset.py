#!/usr/bin/env python3
# load_dataset: perform loading phase for all datasets (non-replayed and replayed) used in evaluation.
# NOTE: replayed traces are not directly used in evaluation, instead we use Zipfian distribution to extract statistics for geo-distributed evaluation.

from .utils.dataset_loader import *

# (0) Global variables and functions

# Check if current machine is a cloud machine
cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
if Common.cur_machine_idx != cloud_machine_idx:
    LogUtil.die(Common.scriptname, "This script is only allowed to run on the cloud machine")

# Used to hint users for keycnt and dataset size
log_filepaths = []

def load_dataset(tmp_workload, tmp_keycnt, tmp_zipf_alpha = None):
    # Get log file name
    tmp_log_filepath = "{}/tmp_dataset_loader_for_{}_key{}.out".format(Common.output_log_dirpath, tmp_workload, tmp_keycnt)
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))
    log_filepaths.append(tmp_log_filepath)

    # Check rocksdb dirpath
    # NOTE: MUST be consistent with getCloudRocksdbDirpath() in src/common/util.c
    tmp_rocksdb_dirpath = "{}/key{}_{}/cloud0.db".format(Common.cloud_rocksdb_basedir, tmp_keycnt, tmp_workload)
    if os.path.exists(tmp_rocksdb_dirpath):
        LogUtil.prompt(Common.scriptname, "Rocksdb dirpath {} already exists, skip dataset loading for non-replayed trace {} with keycnt {}...".format(tmp_rocksdb_dirpath, tmp_workload, tmp_keycnt))
    else:
        # Prepare settings for dataset loader
        tmp_dataset_loader_settings = {
            "keycnt": tmp_keycnt,
            "workload_name": tmp_workload
        }
        if tmp_zipf_alpha is not None:
            tmp_dataset_loader_settings["zipf_alpha"] = tmp_zipf_alpha

        # Launch dataset loader
        dataset_loader_instance = DatasetLoader(dataset_loader_logfile = tmp_log_filepath, **tmp_dataset_loader_settings)
        dataset_loader_instance.run()

# (1) Load dataset into RocksDB with different keycnts for non-replayed traces
nonreplayed_workloads = ["facebook"]
nonreplayed_keycnt_list = [1000000, 2000000, 4000000]
for tmp_workload in nonreplayed_workloads:
    for i in range(len(nonreplayed_keycnt_list)):
        tmp_keycnt = nonreplayed_keycnt_list[i]
        load_dataset(tmp_workload, tmp_keycnt)

# (2) Load dataset into Rocksdb for Zipf-based Facebook CDN workload (non-replayed trace)
tmp_keycnt = 1000000 # 1M by default
tmp_workload = "zipf_facebook"
tmp_zipf_alpha = 0.7 # NOTE: 0.7 is the default skewness of Facebook CDN workload if with Zipf-based generator; workload skewness does NOT affect dataset keys and value sizes
load_dataset(tmp_workload, tmp_keycnt, tmp_zipf_alpha)

# (3) Load dataset into Rocksdb for Zeta-based workloads (non-replayed traces; statistics are extracted from trace files)
zeta_workloads = ["zeta_wikitext", "zeta_wikiimage", "zeta_tencentphoto1", "zeta_tencentphoto2"]
for tmp_workload in zeta_workloads:
    tmp_keycnt = 1000000 # 1M by default
    load_dataset(tmp_workload, tmp_keycnt)

# (4) Hint users to check keycnt and dataset size in log files, and update keycnt in config.json if necessary
# NOTE: we comment the following code as we have already updated config.json with correct keycnt, and calculate cache memory capacity in corresponding exps correctly
#LogUtil.emphasize(Common.scriptname, "Please check keycnt and dataset size in the following log files, and update keycnt in config.json accordingly if necessary:\n{}".format(log_filepaths))






# NOTE: NOT use fbphoto which is NOT open-sourced and hence NO total frequency information for probability calculation and curvefitting -> GDSF+ can achieve 99% global cache hit ratio even if only with 1% memory of 4 edges
# # (2) (OBSOLETE due to unused) Load dataset into Rocksdb for Facebook photo caching (non-replayed trace)
# tmp_keycnt = 1300000 # 1.3M
# tmp_workload = "fbphoto"
# load_dataset(tmp_workload, tmp_keycnt)

# NOTE: NOT simply reply trace files of Wikipedia CDN traces due to without geographical information and incorrect trace partitioning
# # (3) (OBSOLETE due to unused) Load dataset into RocksDB for different replayed traces (NOTE: keycnts have already been updated in config.json after trace preprocessing)
# replayed_workloads = ["wikitext", "wikiimage"]
# for tmp_workload in replayed_workloads:
#     # Get keycnt for the replayed trace
#     tmp_keycnt = JsonUtil.getValueForKeystr(Common.scriptname, "trace_{}_keycnt".format(tmp_workload))
#     if tmp_keycnt <= 0:
#         LogUtil.die(Common.scriptname, "Invalid keycnt {} for replayed trace {}, please preprocess the trace before loading!".format(tmp_keycnt, tmp_workload))
#     load_dataset(tmp_workload, tmp_keycnt)