#!/usr/bin/env python3
# gen_akamai_traces: invoke TRAGEN to generate Akamai CDN traces.

import os
import sys

from ..common import *

tragen_dirpath = os.path.join(Common.lib_dirpath, "tragen")
tragen_cli_filename = "tragen_cli.py"
akamai_trace_dirpath = os.path.join(Common.trace_dirpath, "akamai")

# Check TRAGEN existence
tragen_cli_filepath = os.path.join(tragen_dirpath, tragen_cli_filename)
if not os.path.exists(tragen_cli_filepath):
    LogUtil.die(Common.scriptname, "Please run python3 -m scripts.tools.install_lib to install TRAGEN before generating Akamai's traces!")

# Check Akamai's trace existence
# if os.path.exists(akamai_trace_dirpath):
#     LogUtil.dump(Common.scriptname, "Akamai's CDN traces have been generated into {}!".format(akamai_trace_dirpath))
#     exit(1)

# Try to create directory for Akamai's trace
SubprocessUtil.tryToCreateDirectory(Common.scriptname, akamai_trace_dirpath)

# Generate Akamai trace
akamai_web_trace_config_filepath = os.path.join(Common.proj_dirname, "scripts/tragen/web_config.json")
generate_akamai_trace_cmd = "cd {} && python3 {} -c {} -d {}".format(tragen_dirpath, tragen_cli_filename, akamai_web_trace_config_filepath, akamai_trace_dirpath)
generate_akamai_trace_subprocess = SubprocessUtil.runCmd(generate_akamai_trace_cmd, is_capture_output=False)
if generate_akamai_trace_subprocess.returncode != 0:
    LogUtil.die(Common.scriptname, "failed to generate Akamai web trace (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(generate_akamai_trace_subprocess)))