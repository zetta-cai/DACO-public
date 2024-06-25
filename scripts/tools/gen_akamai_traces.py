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

client_worker_count_list = [4] # NOTE: use 4 now due to 4 cache nodes by default
for tmp_client_worker_count in client_worker_count_list:
    # Check Akamai's trace existence for current client worker count
    # current_akamai_trace_dirpath = os.path.join(akamai_traces_dirpath, str(tmp_client_worker_count))
    # if os.path.exists(current_akamai_trace_dirpath):
    #     LogUtil.dump(Common.scriptname, "Akamai's CDN trace of {} client workers has been generated into {}!".format(tmp_client_worker_count, current_akamai_trace_dirpath))
    #     continue

    # Try to create directory for Akamai's trace of current client worker count
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, current_akamai_trace_dirpath)

    # Generate Akamai trace for current client worker count
    akamai_web_trace_config_filepath = os.path.join(Common.proj_dirname, "scripts/tragen/web_config.json")
    generate_akamai_trace_cmd = "cd {} && python3 {} -c {} -d {} -w {}".format(tragen_dirpath, tragen_cli_filename, akamai_web_trace_config_filepath, current_akamai_trace_dirpath, tmp_client_worker_count)
    generate_akamai_trace_subprocess = SubprocessUtil.runCmd(generate_akamai_trace_cmd, is_capture_output=False)
    if generate_akamai_trace_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to generate Akamai web trace of {} client workers (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(tmp_client_worker_count, generate_akamai_trace_subprocess)))