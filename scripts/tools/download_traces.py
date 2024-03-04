#!/usr/bin/env python3
# download_traces: download replayed traces used in evaluation

import os

from ..common import *

is_clear_tarball = True # whether to clear intermediate tarball files

# Check if current machine is a client machine
client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
if Common.cur_machine_idx not in client_machine_idxes:
    LogUtil.die(Common.scriptname, "This script is only allowed to run on client machines")

# (1) Create trace dirpath if not exist

# Create trace dirpath if not exist
if not os.path.exists(Common.trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as trace dirpath".format(Common.trace_dirpath))
    os.makedirs(Common.trace_dirpath)

# (2) Download Wiki image CDN trace

LogUtil.prompt(Common.scriptname, "download Wiki image CDN trace...")

tmp_trace_dirpath_relative_wikiimage_trace_filepaths_from_json = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikiimage_trace_filepaths")

# Get wikiimage trace filepaths
wikiimage_trace_filepaths = []
for i in range(len(tmp_trace_dirpath_relative_wikiimage_trace_filepaths_from_json)):
    wikiimage_trace_filepaths.append("{}/{}".format(Common.trace_dirpath, tmp_trace_dirpath_relative_wikiimage_trace_filepaths_from_json[i]))

# Create wikiimage trace dirpath if not exist
wikiimage_trace_dirpath = os.path.dirname(wikiimage_trace_filepaths[0])
if not os.path.exists(wikiimage_trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as wikiimage trace dirpath".format(wikiimage_trace_dirpath))
    os.makedirs(wikiimage_trace_dirpath)

# Check each wikiimage trace file
wikiimage_download_url_prefix = "https://analytics.wikimedia.org/published/datasets/caching/2019/upload"
for i in range(len(wikiimage_trace_filepaths)):
    tmp_trace_filepath = wikiimage_trace_filepaths[i]
    if not os.path.exists(tmp_trace_filepath):
        # Download each wikiimage tarball file if not exist
        tmp_trace_filename = os.path.basename(wikiimage_trace_filepaths[i]) # xxx
        tmp_tarball_filename = tmp_trace_filename + ".gz" # xxx.gz
        tmp_tarball_filepath = "{}/{}".format(wikiimage_trace_dirpath, tmp_tarball_filename)
        if not os.path.exists(tmp_tarball_filepath):
            LogUtil.prompt(Common.scriptname, "download {}...".format(tmp_tarball_filepath))
            tmp_download_url = "{}/{}".format(wikiimage_download_url_prefix, tmp_tarball_filename)
            tmp_download_cmd = "cd {} && wget {}".format(wikiimage_trace_dirpath, tmp_download_url)
            tmp_download_subprocess = SubprocessUtil.runCmd(tmp_download_cmd, is_capture_output=False)

        # Decompress each wikiimage trace file if not exist
        LogUtil.prompt(Common.scriptname, "decompress {}...".format(tmp_tarball_filepath))
        tmp_decompress_cmd = "cd {} && gunzip -k {}".format(wikiimage_trace_dirpath, tmp_tarball_filename)
        tmp_decompress_subprocess = SubprocessUtil.runCmd(tmp_decompress_cmd, is_capture_output=False)

        # Clear tarball file if necessary
        if is_clear_tarball:
            LogUtil.prompt(Common.scriptname, "clear {}...".format(tmp_tarball_filepath))
            os.remove(tmp_tarball_filepath)
print("")

# (3) Download Wiki text trace

LogUtil.prompt(Common.scriptname, "download Wiki text trace...")

tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikitext_trace_filepaths")

# Get wikitext trace filepaths
wikitext_trace_filepaths = []
for i in range(len(tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json)):
    wikitext_trace_filepaths.append("{}/{}".format(Common.trace_dirpath, tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json[i]))

# Create wikitext trace dirpath if not exist
wikitext_trace_dirpath = os.path.dirname(wikitext_trace_filepaths[0])
if not os.path.exists(wikitext_trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as wikitext trace dirpath".format(wikitext_trace_dirpath))
    os.makedirs(wikitext_trace_dirpath)

# Check each wikitext trace file
wikitext_download_url_prefix = "https://analytics.wikimedia.org/published/datasets/caching/2019/text"
for i in range(len(wikitext_trace_filepaths)):
    tmp_trace_filepath = wikitext_trace_filepaths[i]
    if not os.path.exists(tmp_trace_filepath):
        # Download each wikitext trace file if not exist
        tmp_trace_filename = os.path.basename(wikitext_trace_filepaths[i]) # xxx
        tmp_tarball_filename = tmp_trace_filename + ".gz" # xxx.gz
        tmp_tarball_filepath = "{}/{}".format(wikitext_trace_dirpath, tmp_tarball_filename)
        if not os.path.exists(tmp_tarball_filepath):
            LogUtil.prompt(Common.scriptname, "download {}...".format(tmp_tarball_filepath))
            tmp_download_url = "{}/{}".format(wikitext_download_url_prefix, tmp_tarball_filename)
            tmp_download_cmd = "cd {} && wget {}".format(wikitext_trace_dirpath, tmp_download_url)
            tmp_download_subprocess = SubprocessUtil.runCmd(tmp_download_cmd, is_capture_output=False)

        # Decompress each wikitext trace file if not exist
        LogUtil.prompt(Common.scriptname, "decompress {}...".format(tmp_tarball_filepath))
        tmp_decompress_cmd = "cd {} && gunzip -k {}".format(wikitext_trace_dirpath, tmp_tarball_filename)
        tmp_decompress_subprocess = SubprocessUtil.runCmd(tmp_decompress_cmd, is_capture_output=False)

        # Clear tarball file if necessary
        if is_clear_tarball:
            LogUtil.prompt(Common.scriptname, "clear {}...".format(tmp_tarball_filepath))
            os.remove(tmp_tarball_filepath)
print("")