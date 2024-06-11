#!/usr/bin/env python3
# decompress_tencent_traces: decompress Tencent photo caching trace files into data/tencent/dataset1/ and data/tencent/dataset2/.

import os
import sys

from ..common import *

tencent_dirpath = os.path.join(Common.trace_dirpath, "tencent") # data/tencent/
test_tarball_filename = "cloud_photos-001.tbz2"
dataset1_dirpath = os.path.join(tencent_dirpath, "dataset1") # data/tencent/dataset1/
dataset2_dirpath = os.path.join(tencent_dirpath, "dataset2") # data/tencent/dataset2/

# (1) Check status

is_dataset_decompressed = os.path.exists(dataset1_dirpath)
if is_dataset_decompressed and not os.path.exists(dataset2_dirpath):
    LogUtil.die(Common.scriptname, "both {} and {} should exist after decompression!".format(dataset1_dirpath, dataset2_dirpath))

is_dataset_downloaded = os.path.exists(os.path.join(tencent_dirpath, test_tarball_filename))

# (2) Decompress Tencent photo caching traces if necessary

if is_dataset_decompressed:
    LogUtil.prompt(Common.scriptname, "Tencent photo caching dataset has been decompressed!")
else: # Decompress datasets
    if not is_dataset_downloaded: # Dataset has not been downloaded
        LogUtil.die(Common.scriptname, "Please download Tencent photo caching dataset first!")
    
    # Dataset has been downloaded -> decompress tbgz files of dataset
    LogUtil.prompt(Common.scriptname, "Decompress Tencent photo caching dataset...")
    for fileidx in range(1, 145): # [1, 144]
        # Pad fileidx into 3-digit string
        fileidx_str = "{:03d}".format(fileidx)

        # Decompress current tbgz file
        tmp_filename = "cloud_photos-{}.tbz2".format(fileidx_str)
        tmp_tbgz_filepath = os.path.join(tencent_dirpath, tmp_filename) # data/tencent/cloud_photos-XXX.tbz2
        LogUtil.prompt(Common.scriptname, "Decompress file {}...".format(tmp_tbgz_filepath))
        tmp_decompress_cmd = "cd {}; tar -jxvf {}".format(tencent_dirpath, tmp_filename)
        tmp_decompress_subprocess = SubprocessUtil.runCmd(tmp_decompress_cmd, is_capture_output = False)
        if tmp_decompress_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to decompress file {} (errmsg: {})".format(tmp_tbgz_filepath, SubprocessUtil.getSubprocessErrstr(tmp_decompress_subprocess)))
        
        # Delete current tbgz file to save space
        # LogUtil.prompt(Common.scriptname, "Delete file {}...".format(tmp_tbgz_filepath))
        # os.remove(tmp_tbgz_filepath)

    # (3) Move trace files to corresponding dataset dirpaths

    # Create data/tencent/dataset1/ and data/tencent/dataset2/ if necessary
    if not os.path.exists(dataset1_dirpath):
        LogUtil.prompt(Common.scriptname, "Create dirpath {} for Tencent photo caching dataset 1...".format(dataset1_dirpath))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, dataset1_dirpath, keep_silent = True)
    if not os.path.exists(dataset2_dirpath):
        LogUtil.prompt(Common.scriptname, "Create dirpath {} for Tencent photo caching dataset 2...".format(dataset2_dirpath))
        SubprocessUtil.tryToCreateDirectory(Common.scriptname, dataset2_dirpath, keep_silent = True)

    # Get all trace files under data/tencent/hust-Tencent_Cloud_Photo_Storage_Cache_Trace/hust-Tencent_Cloud_Photo_Storage_Cache_Trace_1/ and data/tencent/hust-Tencent_Cloud_Photo_Storage_Cache_Trace/hust-Tencent_Cloud_Photo_Storage_Cache_Trace_2/ (including the files under sub-folders)
    tmp_tencent_trace_dirpath = os.path.join(tencent_dirpath, "hust-Tencent_Cloud_Photo_Storage_Cache_Trace")
    tmp_tencent_trace1_dirpath = os.path.join(tmp_tencent_trace_dirpath, "hust-Tencent_Cloud_Photo_Storage_Cache_Trace_1") # data/tencent/hust-Tencent_Cloud_Photo_Storage_Cache_Trace/hust-Tencent_Cloud_Photo_Storage_Cache_Trace_1/
    dataset1_filepaths = []
    for (tmp_dirpath, tmp_dirnames, tmp_filenames) in os.walk(tmp_tencent_trace1_dirpath):
        for tmp_filename in tmp_filenames:
            dataset1_filepaths.append(os.path.join(tmp_dirpath, tmp_filename))
    tmp_tencent_trace2_dirpath = os.path.join(tmp_tencent_trace_dirpath, "hust-Tencent_Cloud_Photo_Storage_Cache_Trace_2") # data/tencent/hust-Tencent_Cloud_Photo_Storage_Cache_Trace/hust-Tencent_Cloud_Photo_Storage_Cache_Trace_2/
    dataset2_filepaths = []
    for (tmp_dirpath, tmp_dirnames, tmp_filenames) in os.walk(tmp_tencent_trace2_dirpath):
        for tmp_filename in tmp_filenames:
            dataset2_filepaths.append(os.path.join(tmp_dirpath, tmp_filename))
    
    # Move trace files to corresponding dataset dirpaths
    for tmp_dataset1_filepath in dataset1_filepaths:
        tmp_filename = os.path.basename(tmp_dataset1_filepath)
        tmp_target_filepath = os.path.join(dataset1_dirpath, tmp_filename)
        SubprocessUtil.moveSrcToTarget(Common.scriptname, tmp_dataset1_filepath, tmp_target_filepath, keep_silent = False)
    for tmp_dataset2_filepath in dataset2_filepaths:
        tmp_filename = os.path.basename(tmp_dataset2_filepath)
        tmp_target_filepath = os.path.join(dataset2_dirpath, tmp_filename)
        SubprocessUtil.moveSrcToTarget(Common.scriptname, tmp_dataset2_filepath, tmp_target_filepath, keep_silent = False)

    # Remove directory hust-Tencent_Cloud_Photo_Storage_Cache_Trace/
    LogUtil.prompt(Common.scriptname, "Remove dirpath {}...".format(tmp_tencent_trace_dirpath))
    tmp_rmdir_cmd = "rm -r {}".format(tmp_tencent_trace_dirpath)
    tmp_rmdir_subprocess = SubprocessUtil.runCmd(tmp_rmdir_cmd)
    if tmp_rmdir_subprocess.returncode != 0:
        LogUtil.die(scriptname, "failed to remove dirpath {} (errmsg: {})".format(tmp_tencent_trace_dirpath, SubprocessUtil.getSubprocessErrstr(tmp_rmdir_subprocess)))