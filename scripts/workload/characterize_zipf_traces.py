#!/usr/bin/env python3
# characterize_traces: characterize replayed trace files by Zipfian distribution to extract characteristics (including Zipfian constant, key size distribution, and value size distribution) for geo-distributed evaluation.

import struct
import gc

from .utils.trace_loader import *
from .utils.zipf_curvefit import *
from ..exps.utils.exputil import *

# Power-law Zipfian constant: [0.7351769996271038, 1.0128733530793717, 0.6645665328056994, 0.6566908883710469, TODO, TODO]
workload_names = [TraceLoader.ZIPF_WIKITEXT_WORKLOADNAME, TraceLoader.ZIPF_WIKIIMAGE_WORKLOADNAME, TraceLoader.ZIPF_TENCENTPHOTO1_WORKLOADNAME, TraceLoader.ZIPF_TENCENTPHOTO2_WORKLOADNAME, TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME, TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME]

# Get dirpath of trace files
client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

# Characterize each workload if necessary
for tmp_workload_name in workload_names:

    is_generate_characteristic_file = False

    # Check whether the workload has been characterized
    tmp_workload_characteristic_filepath = os.path.join(Common.trace_dirpath, "{}.characteristics".format(tmp_workload_name))
    if os.path.exists(tmp_workload_characteristic_filepath):
        # Skip if exist (i.e., already characterized)
        LogUtil.prompt(Common.scriptname, "workload {} has been characterized into {}!".format(tmp_workload_name, tmp_workload_characteristic_filepath))
        is_generate_characteristic_file = False
    else:
        is_generate_characteristic_file = True

    # (1) Characterize the workload and dump the extracted characteristics if necessary

    # If need to generate the characteristics file
    if is_generate_characteristic_file:
        # Get filename list based on the workload name
        if tmp_workload_name == TraceLoader.ZIPF_WIKITEXT_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikitext_trace_filepaths")
        elif tmp_workload_name == TraceLoader.ZIPF_WIKIIMAGE_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikiimage_trace_filepaths")
        elif tmp_workload_name == TraceLoader.ZIPF_TENCENTPHOTO1_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_tencentphoto1_trace_filepaths")
        elif tmp_workload_name == TraceLoader.ZIPF_TENCENTPHOTO2_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_tencentphoto2_trace_filepaths")
        elif tmp_workload_name == TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_twitterkv2_trace_filepaths")
        elif tmp_workload_name == TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_twitterkv4_trace_filepaths")
        else:
            LogUtil.die(Common.scriptname, "unknown workload {}!".format(workload_name))

        # Load statistics (keys, value sizes, and per-key frequency) from trace files of the workload
        tmp_trace_loader = TraceLoader(Common.trace_dirpath, tmp_filename_list, tmp_workload_name)

        # Curvefit the statistics by Zipfian distribution for the workload name
        tmp_sorted_frequency_list = tmp_trace_loader.getSortedFrequencyList()
        tmp_zipf_curvefit = ZipfCurvefit(tmp_sorted_frequency_list)

        # Open the file
        f = open(tmp_workload_characteristic_filepath, "wb")

        # Dump the characteristics (Zipfian constant, key size distribution, and value size distribution) for the workload name
        LogUtil.prompt(Common.scriptname, "dump characteristics for workload {} into {}...".format(tmp_workload_name, tmp_workload_characteristic_filepath))
        tmp_zipf_constant = tmp_zipf_curvefit.getZipfConstant()
        # tmp_zipf_scaling_factor = tmp_zipf_curvefit.getZipfScalingFactor()
        tmp_keysize_histogram = tmp_trace_loader.getKeySizeHistogram()
        tmp_valuesize_histogram = tmp_trace_loader.getValueSizeHistogram()
        os.makedirs(os.path.dirname(tmp_workload_characteristic_filepath), exist_ok=True)
        # Dump Zipfian constant as double in little endian
        f.write(struct.pack("<d", tmp_zipf_constant))
        # # Dump Zipfian scaling factor as double in little endian
        # f.write(struct.pack("<d", tmp_zipf_scaling_factor))
        # Dump key size histogram as uint32_t in little endian
        f.write(struct.pack("<I", len(tmp_keysize_histogram)))
        for i in range(len(tmp_keysize_histogram)):
            f.write(struct.pack("<I", tmp_keysize_histogram[i]))
        # Dump value size histogram as uint32_t in little endian
        f.write(struct.pack("<I", len(tmp_valuesize_histogram)))
        for i in range(len(tmp_valuesize_histogram)):
            f.write(struct.pack("<I", tmp_valuesize_histogram[i]))
        
        # Dump optype ratios for workloads requiring them
        if TraceLoader.needOptypeRatios(tmp_workload_name):
            tmp_read_ratio = tmp_trace_loader.getReadRatio()
            tmp_update_ratio = tmp_trace_loader.getUpdateRatio()
            tmp_delete_ratio = tmp_trace_loader.getDeleteRatio()
            tmp_insert_ratio = tmp_trace_loader.getInsertRatio()

            # Dump read ratio as double in little endian
            f.write(struct.pack("<d", tmp_read_ratio))
            # Dump update ratio as double in little endian
            f.write(struct.pack("<d", tmp_update_ratio))
            # Dump delete ratio as double in little endian
            f.write(struct.pack("<d", tmp_delete_ratio))
            # Dump insert ratio as double in little endian
            f.write(struct.pack("<d", tmp_insert_ratio))

        # Close the file
        f.close()
        
        # Avoid memory overflow
        del tmp_trace_loader
        del tmp_sorted_frequency_list
        del tmp_zipf_curvefit

    # (2) Copy characteristics file to cloud machine if not exist
    if Common.cur_machine_idx == client_machine_idxes[0]: # NOTE: ONLY copy if the current machine is the first client machine to avoid duplicate copying
        if is_generate_characteristic_file:
            # Check characteristics file is generated successfully
            LogUtil.prompt(Common.scriptname, "check if characteristics file {} is generated successfully in current machine...".format(tmp_workload_characteristic_filepath))
            if not os.path.exists(tmp_workload_characteristic_filepath):
                LogUtil.die(Common.scriptname, "Characteristics file not found: {}".format(tmp_workload_characteristic_filepath))
        
        # Try to copy characteristics file to cloud
        ExpUtil.tryToCopyFromCurrentMachineToCloud(tmp_workload_characteristic_filepath)
    
    gc.collect() # Avoid memory overflow