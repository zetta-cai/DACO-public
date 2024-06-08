#!/usr/bin/env python3
# mv_tencent_traces: move Tencent photo caching trace files from tencent/ to data/.

import os
import sys

from ..common import *

tencent_dirname = "tencent"
tmp_tencent_dirpath = os.path.join(Common.proj_dirpath, tencent_dirname) # tencent/

# (1) Move Tencent photo caching dataset 1

# Create data/tencent/dataset1/ if necessary
dataset1_dirpath = os.path.join(Common.trace_dirpath, "{}/dataset1".format(tencent_dirname))
if not os.path.exists(dataset1_dirpath):
    LogUtil.prompt(Common.scriptname, "Create dirpath {} for Tencent photo caching dataset 1...".format(dataset1_dirpath))
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, dataset1_dirpath, keep_silent = True)

for fileidx in range(1, 75): # [1, 74]
    # Pad fileidx into 3-digit string
    fileidx_str = "{:03d}".format(fileidx)

    # Move file from tencent/ into data/tencent/dataset1/
    tmp_filename = "cloud_photos-{}.tbz2".format(fileidx_str)
    tmp_src_filepath = os.path.join(tmp_tencent_dirpath, tmp_filename)
    tmp_target_filepath = os.path.join(dataset1_dirpath, tmp_filename)
    SubprocessUtil.moveSrcToTarget(Common.scriptname, tmp_src_filepath, tmp_target_filepath, keep_silent = False)

# (2) Move Tencent photo caching dataset 2

# Create data/tencent/dataset2/ if necessary
dataset2_dirpath = os.path.join(Common.trace_dirpath, "{}/dataset2".format(tencent_dirname))
if not os.path.exists(dataset2_dirpath):
    LogUtil.prompt(Common.scriptname, "Create dirpath {} for Tencent photo caching dataset 2...".format(dataset2_dirpath))
    SubprocessUtil.tryToCreateDirectory(Common.scriptname, dataset2_dirpath, keep_silent = True)

for fileidx in range(75, 145): # [75, 144]
    # Pad fileidx into 3-digit string
    fileidx_str = "{:03d}".format(fileidx)

    # Move file from tencent/ into data/tencent/dataset2/
    tmp_filename = "cloud_photos-{}.tbz2".format(fileidx_str)
    tmp_src_filepath = os.path.join(tmp_tencent_dirpath, tmp_filename)
    tmp_target_filepath = os.path.join(dataset2_dirpath, tmp_filename)
    SubprocessUtil.moveSrcToTarget(Common.scriptname, tmp_src_filepath, tmp_target_filepath, keep_silent = False)