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
cache_dirs = ["cachelib", "greedydual", "lfu", "lru", "segcache", "lhd", "fifo", "sieve", "s3fifo", "glcache", "lrb", "frozenhot", "arc", "tinylfu", "slru", "bestguess"] # (NOTE: src/cache/covered is for COVERED instead of baseline)
for tmp_cache_dir in cache_dirs:
    exclude_dirs += "|src/cache/{}".format(tmp_cache_dir) # Hacked parts of baselines
exclude_dirs += "|scripts/cachelib|scripts/requirements\.txt|scripts/lrb" # Intermediate files used by scripts
exclude_dirs += ")\""

exclude_files = "\"(src/Makefile" # Root makefile
cache_cpp_files = ["cachelib_local_cache", "fifo_local_cache", "greedy_dual_local_cache", "lfu_local_cache", "lhd_local_cache", "lru_local_cache", "segcache_local_cache", "sieve_local_cache", "s3fifo_local_cache", "glcache_local_cache", "lrb_local_cache", "frozenhot_local_cache", "arc_local_cache", "tinylfu_local_cache", "slru_local_cache", "bestguess_local_cache"] # (NOTE: src/cache/covered_local_cache.* is for COVERED instead of baseline)
for tmp_cache_cpp_file in cache_cpp_files:
    exclude_files += "|src/cache/{0}.c|src/cache/{0}.h".format(tmp_cache_cpp_file) # Redundant code for baseline wrappers
funcparam_cpp_files = ["cache/basic_cache_custom_func_param", "cache/covered_cache_custom_func_param", "cache/cache_custom_func_param_base", "cooperation/basic_cooperation_custom_func_param", "cooperation/covered_cooperation_custom_func_param", "cooperation/cooperation_custom_func_param_base", "edge/basic_edge_custom_func_param", "edge/covered_edge_custom_func_param", "edge/edge_custom_func_param_base"]
for tmp_funcparam_cpp_file in funcparam_cpp_files:
    exclude_files += "|src/{0}.c|src/{0}.h".format(tmp_funcparam_cpp_file) # Function parameters
exclude_files += ")\""

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