#!/usr/bin/env python3
# Common: common global variables

import os
import sys

from .utils import *

class Common:

    # Common variables

    scriptname = sys.argv[0]
    scriptpath = os.path.abspath(scriptname)
    proj_dirname = scriptpath[0:scriptpath.find("/scripts")] # [0, index of "/scripts")

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

    # NOTE: update library_path in config.json for new library installation path if necessary
    tmp_library_dirpath_fromjson = JsonUtil.getValueForKeystr(scriptname, "library_dirpath")
    lib_dirpath = ""
    if tmp_library_dirpath_fromjson[0] == "/": # Absolute path
        lib_dirpath = tmp_library_dirpath_fromjson
    else: # Relative path
        lib_dirpath = "{}/{}".format(proj_dirname, tmp_library_dirpath_fromjson)
    if not os.path.exists(lib_dirpath):
        LogUtil.prompt(scriptname, "{}: Create directory {}...".format(scriptname, lib_dirpath))
        os.mkdir(lib_dirpath)
    #else:
    #    LogUtil.dump(scriptname, "{}: {} exists (third-party libarary dirpath has been created)".format(scriptname, lib_dirpath))

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
                LogUtil.die(scriptname, SubprocessUtil.getSubprocessErrstr(tmp_is_current_machine_subprocess))
            else: # tmp_machine_public_ip not found
                continue
        elif SubprocessUtil.getSubprocessOutputstr(tmp_is_current_machine_subprocess) != "": # tmp_machine_public_ip is found
            cur_machine_idx = tmp_machine_idx
            break
    if cur_machine_idx == -1:
        LogUtil.die(scriptname, "cannot find current machine ip in config.json")

    # NOTE: used by C++ programs evaluator and simulator -> MUST be the same as src/benchmark/evaluator_wrapper.c
    EVALUATOR_FINISH_INITIALIZATION_SYMBOL = "Evaluator initialized"
    EVALUATOR_FINISH_BENCHMARK_SYMBOL = "Evaluator done"

    # NOTE: used by C++ program dataset_loader -> MUST be the same as src/dataset_loader.c
    DATASET_LOADER_FINISH_LOADING_SYMBOL = "Dataset loader done"

    # NOTE: used by C++ program total_statistics_loader -> MUST be the same as src/total_statistics_loader.c
    TOTAL_STATISTICS_LOADER_FINISH_RELOADING_SYMBOL = "Total statistics loader done"

    # NOTE: used by C++ program trace_preprocessor -> MUST be the same as src/trace_preprocessor.c
    TRACE_PREPROCESSOR_FINISH_SYMBOL = "Trace preprocessor done"