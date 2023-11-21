#!/usr/bin/env python3

import os
import sys
import subprocess

from common import *
from utils.util import *

exclude_exts = "d,o,pyc"
exclude_dirs = "\"(src/mk|src/workload/cachebench|src/cache/cachelib|src/cache/lfu|src/cache/lru|src/cache/segcache|src/deprecated|scripts/cachelib|scripts/deprecated)\""
exclude_files = "\"(src/Makefile|scripts/requirements\.txt)\""
# --fullpath add the current working directory (pwd) ahead of --not-match-d and --not-match-f
exclude_command = "--exclude-ext={} --fullpath --not-match-d={} --not-match-f={}".format(exclude_exts, exclude_dirs, exclude_files)

# (1) Count LOC for all C/C++ source code

c_dirs = ["src"]
for tmp_dir in c_dirs:
    prompt(scriptname, "Count LOC for all C/C++ source code in {}...".format(tmp_dir))
    tmp_cloc_cmd = "cloc {} {}".format(exclude_command, tmp_dir)

    tmp_cloc_subprocess = subprocess.run(tmp_cloc_cmd, shell=True)
    if tmp_cloc_subprocess.returncode != 0:
        die(scriptname, "failed to count LOC for all C/C++ source code in {}".format(tmp_dir))
print("")

# (2) Count LOC for design-related C/C++ source code

# "src/message/*/*/covered*"
design_c_dirs = ["src/cache/covered*", "src/core", "src/edge/*/covered*"]
design_c_dir_str = ""
for tmp_dir in design_c_dirs:
    design_c_dir_str += tmp_dir + " "

prompt(scriptname, "Count LOC for design-related C/C++ source code in {}...".format(design_c_dir_str))
tmp_cloc_cmd = "cloc {} {}".format(exclude_command, design_c_dir_str)

tmp_cloc_subprocess = subprocess.run(tmp_cloc_cmd, shell=True)
if tmp_cloc_subprocess.returncode != 0:
    die(scriptname, "failed to count LOC for design-related C/C++ source code in {}".format(tmp_dir))
print("")

# (3) Count LOC for python scripts

py_dirs = ["scripts"]
for tmp_dir in py_dirs:
    prompt(scriptname, "Count LOC for python script code in {}...".format(tmp_dir))
    tmp_cloc_cmd = "cloc {} {}".format(exclude_command, tmp_dir)

    tmp_cloc_subprocess = subprocess.run(tmp_cloc_cmd, shell=True)
    if tmp_cloc_subprocess.returncode != 0:
        die(scriptname, "failed to count python script code in {}".format(tmp_dir))