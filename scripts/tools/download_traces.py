#!/usr/bin/env python3
# download_traces: download replayed traces used in evaluation

import os

from ..common import *

# (1) Create trace dirpath if not exist

tmp_trace_dirpath_from_json = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath")

# Get trace dirpath
trace_dirpath = ""
if tmp_trace_dirpath_from_json[0] == "/": # Absolute path
    trace_dirpath = tmp_trace_dirpath_from_json
else: # Relative path
    trace_dirpath = "{}/{}".format(Common.proj_dirname, tmp_trace_dirpath_from_json)

# Create trace dirpath if not exist
if not os.path.exists(trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as trace dirpath".format(trace_dirpath))
    os.makedirs(trace_dirpath)

# (2) Download Wiki image CDN trace

LogUtil.prompt(Common.scriptname, "download Wiki image CDN trace...")

tmp_trace_dirpath_relative_wikiimage_trace_filespaths_from_json = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikiimage_trace_filespaths")

# Get wikiimage trace filepaths
wikiimage_trace_filepaths = []
for i in range(len(tmp_trace_dirpath_relative_wikiimage_trace_filespaths_from_json)):
    wikiimage_trace_filepaths.append("{}/{}".format(trace_dirpath, tmp_trace_dirpath_relative_wikiimage_trace_filespaths_from_json[i]))

# Create wikiimage trace dirpath if not exist
wikiimage_trace_dirpath = os.path.dirname(wikiimage_trace_filepaths[0])
if not os.path.exists(wikiimage_trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as wikiimage trace dirpath".format(wikiimage_trace_dirpath))
    os.makedirs(wikiimage_trace_dirpath)

# Download each wikiimage trace file if not exist
wikiimage_download_url_prefix = "https://analytics.wikimedia.org/published/datasets/caching/2019/upload"
for i in range(len(wikiimage_trace_filepaths)):
    if not os.path.exists(wikiimage_trace_filepaths[i]):
        LogUtil.prompt(Common.scriptname, "download {}...".format(wikiimage_trace_filepaths[i]))
        tmp_filename = os.path.basename(wikiimage_trace_filepaths[i])
        tmp_download_url = "{}/{}".format(wikiimage_download_url_prefix, tmp_filename)
        tmp_download_cmd = "cd {} && wget {}".format(wikiimage_trace_dirpath, tmp_download_url)
        tmp_download_subprocess = SubprocessUtil.runCmd(tmp_download_cmd, is_capture_output=False)
print("")

# (3) Download Wiki text trace

LogUtil.prompt(Common.scriptname, "download Wiki text trace...")

tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikitext_trace_filepaths")

# Get wikitext trace filepaths
wikitext_trace_filepaths = []
for i in range(len(tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json)):
    wikitext_trace_filepaths.append("{}/{}".format(trace_dirpath, tmp_trace_dirpath_relative_wikitext_trace_filepaths_from_json[i]))

# Create wikitext trace dirpath if not exist
wikitext_trace_dirpath = os.path.dirname(wikitext_trace_filepaths[0])
if not os.path.exists(wikitext_trace_dirpath):
    LogUtil.prompt(Common.scriptname, "create {} as wikitext trace dirpath".format(wikitext_trace_dirpath))
    os.makedirs(wikitext_trace_dirpath)

# Download each wikitext trace file if not exist
wikitext_download_url_prefix = "https://analytics.wikimedia.org/published/datasets/caching/2019/text"
for i in range(len(wikitext_trace_filepaths)):
    if not os.path.exists(wikitext_trace_filepaths[i]):
        LogUtil.prompt(Common.scriptname, "download {}...".format(wikitext_trace_filepaths[i]))
        tmp_filename = os.path.basename(wikitext_trace_filepaths[i])
        tmp_download_url = "{}/{}".format(wikitext_download_url_prefix, tmp_filename)
        tmp_download_cmd = "cd {} && wget {}".format(wikitext_trace_dirpath, tmp_download_url)
        tmp_download_subprocess = SubprocessUtil.runCmd(tmp_download_cmd, is_capture_output=False)
print("")