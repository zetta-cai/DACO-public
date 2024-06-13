#!/usr/bin/env python3
# walk_filepaths_for_config: walk through the filepaths of the given directory and output the filepaths in the format of a list of strings (relative to Common.trace_dirpath, i.e., data/) for config.json.

import os

from ..common import *

# workload_dirname = "wikitext" # For Wikipedia text trace
# workload_dirname = "wikiimage" # For Wikipedia image trace
# workload_dirname = "tencent/dataset1" # For Tencent photo caching dataset 1
workload_dirname = "tencent/dataset2" # For Tencent photo caching dataset 2

# NOTE: we limit the maximize bytes of trace files for each workload due to memory limitation, yet still sufficient to extract the characteristics of the workload
total_filesize_limits = {
    "wikitext": None, # NO limitation (6.3G)
    "wikiimage": None, # NO limitation (90G)
    "tencent/dataset1": 50 * 1000 * 1000 * 1000, # 50G (194G)
    "tencent/dataset2": 50 * 1000 * 1000 * 1000, # 50G (207G)
}
max_total_filesize = total_filesize_limits[workload_dirname]

workload_dirpath = os.path.join(Common.trace_dirpath, workload_dirname) # data/wikitext
total_filesize = 0
is_exceed_max_total_filesize = False
relative_filepath_list = []
for (tmp_dirpath, tmp_dirnames, tmp_filenames) in os.walk(workload_dirpath):
        for tmp_filename in tmp_filenames:
            tmp_filepath = os.path.join(tmp_dirpath, tmp_filename) # data/wikitext/cache-t-00

            # Update and check total file size
            tmp_filesize = os.path.getsize(tmp_filepath)
            total_filesize += tmp_filesize
            if max_total_filesize is not None and total_filesize > max_total_filesize:
                is_exceed_max_total_filesize = True
                break

            # Get file path relative to data/
            tmp_relative_filepath = tmp_filepath.replace(Common.trace_dirpath + "/", "") # Remove data/ from tmp_filepath -> wikitext/cache-t-00
            relative_filepath_list.append(tmp_relative_filepath)
        if is_exceed_max_total_filesize:
            break
relative_filepath_list.sort()

print("# of files: {}".format(len(relative_filepath_list)))

output_str = "["
for i in range(len(relative_filepath_list)):
    output_str += ('\"' + relative_filepath_list[i] + '\"')
    if (i != len(relative_filepath_list) - 1):
        output_str += ", "
    else:
        output_str += "]"
print(output_str)