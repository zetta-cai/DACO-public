#!/usr/bin/env python3

import os
import sys

from .utils import *

# Common variables

scriptname = sys.argv[0]
scriptpath = os.path.abspath(scriptname)
proj_dirname = scriptpath[0:scriptpath.find("/scripts")] # [0, index of "/scripts")

username = os.getenv("SUDO_USER") # Get original username if sudo is used
if username is None: # SUDO_USER is None if sudo is not used
    username = os.getenv("USER") # Get username if sudo is not used (NOTE: USER will be root if sudo is used)

kernel_codename = ""
get_kernel_codename_cmd = "lsb_release -c"
get_kernel_codename_subprocess = runCmd(get_kernel_codename_cmd)
if get_kernel_codename_subprocess.returncode != 0:
    die(scriptname, "failed to get kernel codename (errmsg: {})".format(getSubprocessErrstr(get_kernel_codename_subprocess)))
else:
    kernel_codename = getSubprocessOutputstr(get_kernel_codename_subprocess).splitlines()[0].split()[-1]

# NOTE: update library_path in config.json for new library installation path if necessary
library_dirpath_fromjson = getValueForKeystr(scriptname, "library_dirpath")
lib_dirpath = ""
if library_dirpath_fromjson[0] == "/": # Absolute path
    lib_dirpath = library_dirpath_fromjson
else: # Relative path
    lib_dirpath = "{}/{}".format(proj_dirname, library_dirpath_fromjson)
if not os.path.exists(lib_dirpath):
    prompt(scriptname, "{}: Create directory {}...".format(scriptname, lib_dirpath))
    os.mkdir(lib_dirpath)
#else:
#    dump(scriptname, "{}: {} exists (third-party libarary dirpath has been created)".format(scriptname, lib_dirpath))

# Get current machine index for passfree SSH configuraiton and launch prototype
prompt(scriptname, "get current machine index based on ifconfig")
cur_machine_idx = -1
for tmp_machine_idx in range(len(config_jsonobj["physical_machines"])):
    tmp_machine_ip = config_jsonobj["physical_machines"][tmp_machine_idx]["ipstr"]
    is_current_machine_cmd = "ifconfig | grep -w " + tmp_machine_ip # -w means whole word matching
    is_current_machine_subprocess = runCmd(is_current_machine_cmd)
    if is_current_machine_subprocess.returncode != 0: # Error or tmp_machine_ip not found
        if getSubprocessErrstr(is_current_machine_subprocess) != "": # Error
            die(scriptname, getSubprocessErrstr(is_current_machine_subprocess))
        else: # tmp_machine_ip not found
            continue
    elif getSubprocessOutputstr(is_current_machine_subprocess) != "": # tmp_machine_ip is found
        cur_machine_idx = tmp_machine_idx
        break
if cur_machine_idx == -1:
    die(scriptname, "cannot find current machine ip in config.json")