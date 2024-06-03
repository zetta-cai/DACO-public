#!/usr/bin/env python3
# characterize_traces: characterize replayed trace files by Zipfian distribution to extract characteristics (including Zipfian constant, key size distribution, and value size distribution) for geo-distributed evaluation.

import struct

from .utils.trace_loader import *
from .utils.zipf_curvefit import *

workload_names = [TraceLoader.WIKITEXT_WORKLOADNAME, TraceLoader.WIKIIMAGE_WORKLOADNAME]

# Get dirpath of trace files
tmp_trace_dirpath = JsonUtil.getFullPathForKeystr(Common.scriptname, "trace_dirpath", Common.proj_dirname)

# Characterize each workload if necessary
for tmp_workload_name in workload_names:
    # Check whether the workload has been characterized
    tmp_workload_characteristic_filepath = os.path.join(tmp_trace_dirpath, "{}.characteristics".format(tmp_workload_name))
    if os.path.exists(tmp_workload_characteristic_filepath):
        # Skip if exist (i.e., already characterized)
        LogUtil.prompt(Common.scriptname, "workload {} has been characterized into {}!".format(tmp_workload_name, tmp_workload_characteristic_filepath))
        continue

    # Get filename list based on the workload name
    if tmp_workload_name == TraceLoader.WIKITEXT_WORKLOADNAME:
        tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikitext_trace_filepaths")
    elif tmp_workload_name == TraceLoader.WIKIIMAGE_WORKLOADNAME:
        tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikiimage_trace_filepaths")
    else:
        LogUtil.die(Common.scriptname, "unknown workload {}!".format(workload_name))

    # Load statistics (keys, value sizes, and per-key frequency) from trace files of the workload
    tmp_trace_loader = TraceLoader(tmp_trace_dirpath, tmp_filename_list, tmp_workload_name)

    # Curvefit the statistics by Zipfian distribution for the workload name
    tmp_sorted_frequency_list = tmp_trace_loader.getSortedFrequencyList()
    tmp_zipf_curvefit = ZipfCurvefit(tmp_sorted_frequency_list)

    # Dump the characteristics (Zipfian constant, key size distribution, and value size distribution) for the workload name
    LogUtil.prompt(Common.scriptname, "dump characteristics for workload {} into {}...".format(tmp_workload_name, tmp_workload_characteristic_filepath))
    tmp_zipf_constant = tmp_zipf_curvefit.getZipfConstant()
    tmp_keysize_histogram = tmp_trace_loader.getKeySizeHistogram()
    tmp_valuesize_histogram = tmp_trace_loader.getValueSizeHistogram()
    os.makedirs(os.path.dirname(tmp_workload_characteristic_filepath), exist_ok=True)
    with open(tmp_workload_characteristic_filepath, "wb") as f:
        # Dump Zipfian constant as double in little endian
        f.write(struct.pack("<d", tmp_zipf_constant))
        # Dump key size histogram as uint32_t in little endian
        f.write(struct.pack("<I", len(tmp_keysize_histogram)))
        for i in range(len(tmp_keysize_histogram)):
            f.write(struct.pack("<I", tmp_keysize_histogram[i]))
        # Dump value size histogram as uint32_t in little endian
        f.write(struct.pack("<I", len(tmp_valuesize_histogram)))
        for i in range(len(tmp_valuesize_histogram)):
            f.write(struct.pack("<I", tmp_valuesize_histogram[i]))