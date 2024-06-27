#!/usr/bin/env python3
# (OBSOLETE due to unused) proprocess_traces: preprocess trace files to dump partial workload and dataset items.

from ..common import *
from .utils.exputil import *
from .utils.trace_preprocessor import *

# Get configuration settings from config.json
client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")
trace_sample_opcnt = JsonUtil.getValueForKeystr(Common.scriptname, "trace_sample_opcnt")

# Check if current machine is a client machine
if Common.cur_machine_idx not in client_machine_idxes:
    LogUtil.die(Common.scriptname, "This script is only allowed to run on client machines")

# Used to hint users for keycnt and dataset size
log_filepaths = []

# Preprocess all replayed traces
replayed_workloads = ["wikitext", "wikiimage"]
for i in range(len(replayed_workloads)):
    tmp_workload = replayed_workloads[i]

    # Get log file name
    tmp_log_filepath = "{}/tmp_trace_preprocessor_for_{}.out".format(Common.output_log_dirpath, tmp_workload)
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, os.path.dirname(tmp_log_filepath))
    log_filepaths.append(tmp_log_filepath)

    # Get settings for trace preprocessor
    tmp_settings = {
        "workload_name": tmp_workload
    }

    # NOTE: MUST be the same as dataset filepath in src/common/util.c
    tmp_dataset_filepath = "{}/{}.dataset.{}".format(Common.trace_dirpath, tmp_workload, trace_sample_opcnt)
    tmp_workload_filepath = "{}/{}.workload.{}".format(Common.trace_dirpath, tmp_workload, trace_sample_opcnt)

    # (1) Launch trace preprocessor (will generate dataset file)
    is_generate_dataset_file = False
    if os.path.exists(tmp_dataset_filepath) and os.path.exists(tmp_workload_filepath):
        LogUtil.prompt(Common.scriptname, "Dataset file {} and workload file {} already exist, skip trace preprocessing...".format(tmp_dataset_filepath, tmp_workload_filepath))
    else:
        LogUtil.prompt(Common.scriptname, "preprocess workload {} in current machine...".format(tmp_workload))
        tmp_trace_preprocessor = TracePreprocessor(trace_preprocessor_logfile = tmp_log_filepath, **tmp_settings)
        tmp_trace_preprocessor.run()

        is_generate_dataset_file = True

    # (2) Copy dataset file to cloud machine if not exist
    if Common.cur_machine_idx == client_machine_idxes[0]: # NOTE: ONLY copy if the current machine is the first client machine to avoid duplicate copying
        if is_generate_dataset_file:
            # Check dataset file is generated successfully
            LogUtil.prompt(Common.scriptname, "check if dataset file {} is generated successfully in current machine...".format(tmp_dataset_filepath))
            if not os.path.exists(tmp_dataset_filepath):
                LogUtil.die(Common.scriptname, "Dataset file not found: {}".format(tmp_dataset_filepath))
        
        # Try to copy dataset file to cloud
        ExpUtil.tryToCopyFromCurrentMachineToCloud(tmp_dataset_filepath)

# (3) Hint users to check keycnt and dataset size in log files, and update keycnt in config.json if necessary
# NOTE: we comment the following code as we have already updated config.json with correct keycnt, and calculate cache memory capacity in corresponding exps correctly
#LogUtil.emphasize(Common.scriptname, "Please check keycnt and dataset size in the following log files, and update keycnt in config.json accordingly if necessary:\n{}".format(log_filepaths))