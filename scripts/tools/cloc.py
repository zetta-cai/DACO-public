#!/usr/bin/env python3
# cloc: count lines of C++ code and scripts

import os
import sys
import subprocess

from ..common import *

exclude_exts = "d,o,pyc"
exclude_dirs = "\"(src/mk" # Module makefiles
exclude_dirs += "|src/deprecated|scripts/deprecated" # Deprecated source code and scripts
exclude_dirs += "|src/workload/cachebench" # Workload source code
exclude_dirs += "|src/cache/cachelib|src/cache/greedydual|src/cache/lfu|src/cache/lru|src/cache/segcache|src/cache/lhd|src/cache/fifo|src/cache/sieve" # Baselines (NOTE: src/cache/covered is for COVERED instead of baseline)
exclude_dirs += "|scripts/cachelib|scripts/requirements\.txt)\"" # Intermediate files used by scripts
exclude_files = "\"(src/Makefile)\"" # Root makefile

# TODO: Reduce redundant code
#exclude_files = "\"(src/Makefile" # Root makefile
#exclude_files += "|src/cache/cachelib_local_cache.*|src/cache/fifo_local_cache.*|src/cache/greedy_dual_local_cache.*|src/cache/lfu_local_cache.*|src/cache/lhd_local_cache.*|src/cache/lru_local_cache.*|src/cache/segcache_local_cache.*|src/cache/sieve_local_cache.*)\"" # Redundant code for baseline wrappers

# --fullpath add the current working directory (pwd) ahead of --not-match-d and --not-match-f
exclude_command = "--exclude-ext={} --fullpath --not-match-d={} --not-match-f={}".format(exclude_exts, exclude_dirs, exclude_files)

# (1) Count LOC for all C/C++ source code

c_dirs = ["src"]
for tmp_dir in c_dirs:
    LogUtil.prompt(Common.scriptname, "Count LOC for all C/C++ source code in {}...".format(tmp_dir))
    tmp_cloc_cmd = "cloc {} {}".format(exclude_command, tmp_dir)

    tmp_cloc_subprocess = SubprocessUtil.runCmd(tmp_cloc_cmd)
    if tmp_cloc_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to count LOC for all C/C++ source code in {} (errmsg: {})".format(tmp_dir, SubprocessUtil.getSubprocessErrstr(tmp_cloc_subprocess)))
    else:
        tmp_outputstr = SubprocessUtil.getSubprocessOutputstr(tmp_cloc_subprocess)
        print(tmp_outputstr)
print("")

# (2) Count LOC for design-related C/C++ source code

# "src/message/*/*/covered*"
design_c_dirs = ["src/cache/covered*", "src/core", "src/edge/*/covered*"]
design_c_dir_str = ""
for tmp_dir in design_c_dirs:
    design_c_dir_str += tmp_dir + " "

LogUtil.prompt(Common.scriptname, "Count LOC for design-related C/C++ source code in {}...".format(design_c_dir_str))
tmp_cloc_cmd = "cloc {} {}".format(exclude_command, design_c_dir_str)

tmp_cloc_subprocess = SubprocessUtil.runCmd(tmp_cloc_cmd)
if tmp_cloc_subprocess.returncode != 0:
    LogUtil.die(Common.scriptname, "failed to count LOC for design-related C/C++ source code in {} (errmsg: {})".format(tmp_dir, SubprocessUtil.getSubprocessErrstr(tmp_cloc_subprocess)))
else:
    tmp_outputstr = SubprocessUtil.getSubprocessOutputstr(tmp_cloc_subprocess)
    print(tmp_outputstr)
print("")

# (3) Count LOC for python scripts

py_dirs = ["scripts"]
for tmp_dir in py_dirs:
    LogUtil.prompt(Common.scriptname, "Count LOC for python script code in {}...".format(tmp_dir))
    tmp_cloc_cmd = "cloc {} {}".format(exclude_command, tmp_dir)

    tmp_cloc_subprocess = SubprocessUtil.runCmd(tmp_cloc_cmd)
    if tmp_cloc_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to count python script code in {} (errmsg: {})".format(tmp_dir, SubprocessUtil.getSubprocessErrstr(tmp_cloc_subprocess)))
    else:
        tmp_outputstr = SubprocessUtil.getSubprocessOutputstr(tmp_cloc_subprocess)
        print(tmp_outputstr)