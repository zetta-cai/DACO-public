#!/usr/bin/env python3
# gen_akamai_traces: invoke TRAGEN to generate Akamai CDN traces.

import os
import sys

from ..common import *

tragen_dirpath = os.path.join(Common.lib_dirpath, "tragen")
tragen_cli_filename = "tragen_cli.py"
akamai_traces_dirpath = os.path.join(Common.trace_dirpath, "akamai")

# Check TRAGEN existence
tragen_cli_filepath = os.path.join(tragen_dirpath, tragen_cli_filename)
if not os.path.exists(tragen_cli_filepath):
    LogUtil.die(Common.scriptname, "Please run python3 -m scripts.tools.install_lib to install TRAGEN before generating Akamai's traces!")

# Try to create directory for Akamai's traces
SubprocessUtil.tryToCreateDirectory(Common.scriptname, akamai_traces_dirpath)

client_worker_count_list = [4] # NOTE: use 4 now due to 4 cache nodes by default
dataset_objcnt = 1 * 1000 * 1000 # Use 1M dataset objcnt by default for fair comparison
for tmp_client_worker_count in client_worker_count_list:

    # Generate Akamai trace for current client worker count
    # E.g., for 1M dataset and 4 client workers, TRAGEN will generate data/akamai/dataset1000000_workercnt4/worker[0-3]_sequence.txt and data/akamai/dataset1000000.txt
    akamai_web_trace_config_filepath = os.path.join(Common.proj_dirname, "scripts/tragen/web_config.json")
    generate_akamai_trace_cmd = "cd {} && python3 {} -c {} -o {} -w {} -d {}".format(tragen_dirpath, tragen_cli_filename, akamai_web_trace_config_filepath, akamai_traces_dirpath, tmp_client_worker_count, dataset_objcnt)
    generate_akamai_trace_subprocess = SubprocessUtil.runCmd(generate_akamai_trace_cmd, is_capture_output=False)
    if generate_akamai_trace_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to generate Akamai web trace of {} client workers (errmsg: {})".format(tmp_client_worker_count, SubprocessUtil.getSubprocessErrstr(generate_akamai_trace_subprocess)))