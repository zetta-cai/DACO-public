#!/usr/bin/env python3
# walk_filepaths_for_config: walk through the filepaths of the given directory and output the filepaths in the format of a list of strings (relative to Common.trace_dirpath, i.e., data/) for config.json.

import os

from ..common import *

# workload_dirname = "wikitext" # For Wikipedia text trace
# workload_dirname = "wikiimage" # For Wikipedia image trace
# workload_dirname = "tencent/dataset1" # For Tencent photo caching dataset 1
workload_dirname = "tencent/dataset2" # For Tencent photo caching dataset 2

workload_dirpath = os.path.join(Common.trace_dirpath, workload_dirname) # data/wikitext
relative_filepath_list = []
for (tmp_dirpath, tmp_dirnames, tmp_filenames) in os.walk(workload_dirpath):
        for tmp_filename in tmp_filenames:
            tmp_filepath = os.path.join(tmp_dirpath, tmp_filename) # data/wikitext/cache-t-00
            tmp_relative_filepath = tmp_filepath.replace(Common.trace_dirpath + "/", "") # Remove data/ from tmp_filepath -> wikitext/cache-t-00
            relative_filepath_list.append(tmp_relative_filepath)
relative_filepath_list.sort()

output_str = "["
for i in range(len(relative_filepath_list)):
    output_str += ('\"' + relative_filepath_list[i] + '\"')
    if (i != len(relative_filepath_list) - 1):
        output_str += ", "
    else:
        output_str += "]"
print(output_str)