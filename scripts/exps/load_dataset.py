#!/usr/bin/env python3
# load_dataset: perform loading phase for all datasets (non-replayed and replayed) used in evaluation.

from .utils.dataset_loader import *

# Check if current machine is a cloud machine
cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
if Common.cur_machine_idx != cloud_machine_idx:
    LogUtil.die(Common.scriptname, "This script is only allowed to run on the cloud machine")

# Used to hint users for keycnt and dataset size
log_filepaths = []

# (1) Load dataset into RocksDB with different keycnts for non-replayed traces
nonreplayed_workloads = ["facebook"]
nonreplayed_keycnt_list = [1000000, 2000000, 4000000]
for tmp_workload in nonreplayed_workloads:
    for i in range(len(nonreplayed_keycnt_list)):
        tmp_keycnt = nonreplayed_keycnt_list[i]

        # Get log file name
        tmp_log_filepath = "{}/tmp_dataset_loader_for_{}_key{}.out".format(Common.output_log_dirpath, tmp_workload, tmp_keycnt)
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

            # Launch dataset loader
            dataset_loader_instance = DatasetLoader(dataset_loader_logfile = tmp_log_filepath, **tmp_dataset_loader_settings)
            dataset_loader_instance.run()

# (2) Load dataset into RocksDB for different replayed traces (NOTE: keycnts have already been updated in config.json after trace preprocessing)
replayed_workloads = ["wikitext", "wikiimage"]
for tmp_workload in replayed_workloads:
    # Get keycnt for the replayed trace
    tmp_keycnt = JsonUtil.getValueForKeystr(Common.scriptname, "trace_{}_keycnt".format(tmp_workload))
    if tmp_keycnt <= 0:
        LogUtil.die(Common.scriptname, "Invalid keycnt {} for replayed trace {}, please preprocess the trace before loading!".format(tmp_keycnt, tmp_workload)

    # Get log file name
    tmp_log_filepath = "{}/tmp_dataset_loader_for_{}_key{}.out".format(Common.output_log_dirpath, tmp_workload, tmp_keycnt)
    log_filepaths.append(tmp_log_filepath)

    # Check rocksdb dirpath
    # NOTE: MUST be consistent with getCloudRocksdbDirpath() in src/common/util.c
    tmp_rocksdb_dirpath = "{}/key{}_{}/cloud0.db".format(Common.cloud_rocksdb_basedir, tmp_keycnt, tmp_workload)
    if os.path.exists(tmp_rocksdb_dirpath):
        LogUtil.prompt(Common.scriptname, "Rocksdb dirpath {} already exists, skip dataset loading for eplayed trace {} with keycnt {}...".format(tmp_rocksdb_dirpath, tmp_workload, tmp_keycnt))
    else:
        # Prepare settings for dataset loader
        tmp_dataset_loader_settings = {
            "keycnt": tmp_keycnt, # NOT used by CLI module in C++
            "workload_name": tmp_workload
        }

        # Launch dataset loader
        dataset_loader_instance = DatasetLoader(dataset_loader_logfile = tmp_log_filepath, **tmp_dataset_loader_settings)
        dataset_loader_instance.run()

# (3) Hint users to check keycnt and dataset size in log files, and update keycnt in config.json if necessary
# NOTE: we comment the following code as we have already updated config.json with correct keycnt, and calculate cache memory capacity in corresponding exps correctly
#LogUtil.emphasize(Common.scriptname, "Please check keycnt and dataset size in the following log files, and update keycnt in config.json accordingly if necessary:\n{}".format(log_filepaths))