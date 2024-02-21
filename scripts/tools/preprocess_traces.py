#!/usr/bin/env python3
# cleanup_testbed: kill all related threads launched in testbed.

from ..common import *
from ..exps.utils.exputil import *

replayed_workloads = ["wikiimage", "wikitext"]
log_filenames = ["tmp_trace_preprocessor_for_{}.out".format(tmp_workload) for tmp_workload in replayed_workloads]

for tmp_workload in replayed_workloads:
    tmp_settings = {
        "workload_name": tmp_workload
    }

    # (1) Launch trace preprocessor
    # TODO: Pass log file name
    tmp_trace_preprocessor = TracePreprocessor(**tmp_settings)
    tmp_trace_preprocessor.run()

    # (2) TODO: If the current machine is the first client machine (avoid duplicate copy), copy dataset file to cloud machine if not exist
    # TODO: Check dataset file is generated successfully
    # TODO: Check if cloud machine has dataset file
    # TODO: If cloud machine does NOT have the dataset file, send it to cloud machine

# (3) TODO: Hint users to check keycnt and total opcnt in log files, and update config.json if necessary
# NOTE: we comment the following code as we have already updated config.json with correct keycnt and total opcnt