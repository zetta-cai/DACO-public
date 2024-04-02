#!/usr/bin/env python3
# Common: common global variables

import os
import sys

from .utils import *

class Common:

    # Common variables

    ## (1) General settings

    scriptname = sys.argv[0]
    scriptpath = os.path.abspath(scriptname)

    username = os.getenv("SUDO_USER") # Get original username if sudo is used
    if username is None: # SUDO_USER is None if sudo is not used
        username = os.getenv("USER") # Get username if sudo is not used (NOTE: USER will be root if sudo is used)
    sshkey_name = "id_rsa_for_covered"
    sshkey_filepath = "/home/{}/.ssh/{}".format(username, sshkey_name)

    kernel_codename = ""
    tmp_get_kernel_codename_cmd = "lsb_release -c"
    tmp_get_kernel_codename_subprocess = SubprocessUtil.runCmd(tmp_get_kernel_codename_cmd)
    if tmp_get_kernel_codename_subprocess.returncode != 0:
        LogUtil.die(scriptname, "failed to get kernel codename (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(tmp_get_kernel_codename_subprocess)))
    else:
        kernel_codename = SubprocessUtil.getSubprocessOutputstr(tmp_get_kernel_codename_subprocess).splitlines()[0].split()[-1]
    
    ## (2) Path settings
    
    proj_dirname = scriptpath[0:scriptpath.find("/scripts")] # [0, index of "/scripts")

    # NOTE: update library_path in config.json for new library installation path if necessary
    lib_dirpath = JsonUtil.getFullPathForKeystr(scriptname, "library_dirpath", proj_dirname)
    # NOTE: all roles (e.g., client/edge/cloud/evaluator) need libraries -> create library dirpath here
    SubprocessUtil.tryToCreateDirectory(scriptname, lib_dirpath)

    # NOTE: update trace_dirpath in config.json for new trace directory path if necessary
    # NOTE: NOT all roles need trace dirpath (ONLY clients and cloud need) -> NOT create trace dirpath here
    trace_dirpath = JsonUtil.getFullPathForKeystr(scriptname, "trace_dirpath", proj_dirname)

    # NOTE: update cloud_rocksdb_basedir in config.json for new rocksdb base directory path if necessary
    # NOTE: NOT all roles need rocksdb base dirpath (ONLY cloud needs) -> NOT create rocksdb base dirpath here
    cloud_rocksdb_basedir = JsonUtil.getFullPathForKeystr(scriptname, "cloud_rocksdb_basedir", proj_dirname)

    # NOTE: update output_dirpath in config.json for intermediate files (e.g., evaluator statistics and evaluation logs) if necessary
    # NOTE: NOT all roles need output dirpath (ONLY clients, cloud, and evaluator needs) -> NOT create output dirpath here
    output_dirpath = JsonUtil.getFullPathForKeystr(scriptname, "output_dirpath", proj_dirname)
    output_log_dirpath = "{}/log".format(output_dirpath)

    ## (3) Machine settings

    # Get current machine index for passfree SSH configuraiton and launch prototype
    LogUtil.prompt(scriptname, "get current machine index based on ifconfig")
    cur_machine_idx = -1
    tmp_physical_machines = JsonUtil.getValueForKeystr(scriptname, "physical_machines")
    for tmp_machine_idx in range(len(tmp_physical_machines)):
        tmp_machine_public_ip = tmp_physical_machines[tmp_machine_idx]["public_ipstr"]
        tmp_is_current_machine_cmd = "ifconfig | grep -w " + tmp_machine_public_ip # -w means whole word matching
        tmp_is_current_machine_subprocess = SubprocessUtil.runCmd(tmp_is_current_machine_cmd)
        if tmp_is_current_machine_subprocess.returncode != 0: # Error or tmp_machine_public_ip not found
            if SubprocessUtil.getSubprocessErrstr(tmp_is_current_machine_subprocess) != "": # Error
                LogUtil.die(scriptname, SubprocessUtil.getSubprocessErrstr(tmp_is_current_machine_subprocess)) # Dump error string
        elif SubprocessUtil.getSubprocessOutputstr(tmp_is_current_machine_subprocess) != "": # tmp_machine_public_ip is found
            cur_machine_idx = tmp_machine_idx
            break
        
        tmp_machine_private_ip = tmp_physical_machines[tmp_machine_idx]["private_ipstr"]
        tmp_is_current_machine_cmd = "ifconfig | grep -w " + tmp_machine_private_ip # -w means whole word matching
        tmp_is_current_machine_subprocess = SubprocessUtil.runCmd(tmp_is_current_machine_cmd)
        if tmp_is_current_machine_subprocess.returncode != 0: # Error or tmp_machine_private_ip not found
            if SubprocessUtil.getSubprocessErrstr(tmp_is_current_machine_subprocess) != "": # Error
                LogUtil.die(scriptname, SubprocessUtil.getSubprocessErrstr(tmp_is_current_machine_subprocess)) # Dump error string
        elif SubprocessUtil.getSubprocessOutputstr(tmp_is_current_machine_subprocess) != "": # tmp_machine_private_ip is found
            cur_machine_idx = tmp_machine_idx
            break
    # NOTE: comment for real-network testbed, as we install libs before updating the config json file
    # if cur_machine_idx == -1:
    #     LogUtil.die(scriptname, "cannot find current machine ip in config.json")
    
    ## (4) Symbol settings

    # NOTE: used by C++ programs evaluator and simulator -> MUST be the same as src/benchmark/evaluator_wrapper.c
    EVALUATOR_FINISH_INITIALIZATION_SYMBOL = "Evaluator initialized"
    EVALUATOR_FINISH_BENCHMARK_SYMBOL = "Evaluator done"

    # NOTE: used by C++ program dataset_loader -> MUST be the same as src/dataset_loader.c
    DATASET_LOADER_FINISH_LOADING_SYMBOL = "Dataset loader done"

    # NOTE: used by C++ program total_statistics_loader -> MUST be the same as src/total_statistics_loader.c
    TOTAL_STATISTICS_LOADER_FINISH_RELOADING_SYMBOL = "Total statistics loader done"

    # NOTE: used by C++ program trace_preprocessor -> MUST be the same as src/trace_preprocessor.c
    TRACE_PREPROCESSOR_FINISH_SYMBOL = "Trace preprocessor done"

    ## (5) Experiment settings

    exp_round_number = 4 # Run each experiment for exp_round_number times

    ## (6) Real-network settings

    # (Silicon Valley) Initialized single-trip time (RTT / 2) (TODO: update based on your real-network testbed)
    # NOTE: used for the five rounds respectively
    alicloud_avg_clientedge_latency_us_list = [386, 782, 765, 765]
    alicloud_avg_crossedge_latency_us_list = [10150, 11305, 11165, 10755]
    alicloud_avg_edgecloud_latency_us_list = [74750, 81000, 80875, 79875]