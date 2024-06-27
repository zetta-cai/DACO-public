#!/usr/bin/env python3
# gen_akamai_traces: invoke TRAGEN to generate Akamai CDN traces.
# NOTE: we hack some parts of TRAGEN (see scripts/tragen/) to (i) use different random seeds for various workload sequences across multiple clients; (ii) scale stack distances for the given dataset object count (original stack distances is fixed for 70M); and (iii) continuously generate workload requests for the given trace length under a fixed set of dataset objects (original generation dynamically adds more dataset objects).

import os
import sys

from ..common import *
from ..exps.utils.exputil import *

# Settings
client_worker_count_list = [4] # NOTE: use 4 now due to 4 cache nodes by default
dataset_objcnt = 1 * 1000 * 1000 # Use 1M dataset objcnt by default for fair comparison
# NOTE: akamai_trace_dirpaths and akamai_config_filenames (under scripts/tragen/) are one-to-one mapping
akamai_trace_dirnames = ["akamaiweb", "akamaivideo"]
akamai_trace_dirpaths = []
for tmp_akamai_trace_dirname in akamai_trace_dirnames:
    akamai_trace_dirpaths.append(os.path.join(Common.trace_dirpath, tmp_akamai_trace_dirname)) # E.g., /xxx/data/akamaiweb
akamai_config_filenames = ["web_config.json", "video_config.json"]
akamai_config_filepaths = []
for tmp_akamai_config_filename in akamai_config_filenames:
    akamai_config_filepaths.append(os.path.join(Common.proj_dirname, "scripts/tragen", tmp_akamai_config_filename)) # E.g., /xxx/scripts/tragen/web_config.json

# Configurations
client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
tragen_dirpath = os.path.join(Common.lib_dirpath, "tragen") # lib/tragen
tragen_cli_filename = "tragen_cli.py"

# Check TRAGEN existence
tragen_cli_filepath = os.path.join(tragen_dirpath, tragen_cli_filename)
if not os.path.exists(tragen_cli_filepath):
    LogUtil.die(Common.scriptname, "Please run python3 -m scripts.tools.install_lib to install TRAGEN before generating Akamai's traces!")

# For each Akamai's trace type
for tmp_akamai_trace_idx in range(len(akamai_trace_dirpaths)):
    tmp_akamai_trace_dirname = akamai_trace_dirnames[tmp_akamai_trace_idx]
    tmp_akamai_trace_dirpath = akamai_trace_dirpaths[tmp_akamai_trace_idx]
    tmp_akamai_config_filepath = akamai_config_filepaths[tmp_akamai_trace_idx]

    # Try to create directory for current Akamai's trace type
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, tmp_akamai_trace_dirpath)

    # For each client worker count setting
    for tmp_client_worker_count in client_worker_count_list:

        # Generate current Akamai trace type for current client worker count
        # E.g., for 1M dataset and 4 client workers, TRAGEN will generate data/akamaiweb/dataset1000000_workercnt4/worker[0-3]_sequence.txt and data/akamaiweb/dataset1000000.txt
        generate_akamai_trace_cmd = "cd {} && python3 {} -c {} -o {} -w {} -d {}".format(tragen_dirpath, tragen_cli_filename, tmp_akamai_config_filepath, tmp_akamai_trace_dirpath, tmp_client_worker_count, dataset_objcnt)
        generate_akamai_trace_subprocess = SubprocessUtil.runCmd(generate_akamai_trace_cmd, is_capture_output=False)
        if generate_akamai_trace_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, "failed to generate Akamai trace of {} for {} client workers (errmsg: {})".format(tmp_akamai_trace_dirname, tmp_client_worker_count, SubprocessUtil.getSubprocessErrstr(generate_akamai_trace_subprocess)))
    
    # Copy dataset file (only related to trace type and dataset objcnt) to cloud machine if not exist
    tmp_akamai_dataset_filepath = os.path.join(tmp_akamai_trace_dirpath, "dataset{}.txt".format(dataset_objcnt)) # E.g., data/akamaiweb/dataset1000000.txt
    if Common.cur_machine_idx == client_machine_idxes[0]: # NOTE: ONLY copy if the current machine is the first client machine to avoid duplicate copying
        ExpUtil.tryToCopyFromCurrentMachineToCloud(tmp_akamai_dataset_filepath)