#!/usr/bin/env python3
# characterize_traces: characterize replayed trace files by Zipfian distribution to extract characteristics (including Zipfian constant, key size distribution, and value size distribution) for geo-distributed evaluation.

import struct
import gc

from .utils.trace_loader import *
from .utils.zipf_curvefit import *
from ..exps.utils.exputil import *

# Zipfian constant: [1.012, 1.029, 1.0095, TODO]
workload_names = [TraceLoader.WIKITEXT_WORKLOADNAME, TraceLoader.WIKIIMAGE_WORKLOADNAME, TraceLoader.TENCENTPHOTO1_WORKLOADNAME, TraceLoader.TENCENTPHOTO2_WORKLOADNAME]

# Get dirpath of trace files
client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
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
        if tmp_workload_name == TraceLoader.WIKITEXT_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikitext_trace_filepaths")
        elif tmp_workload_name == TraceLoader.WIKIIMAGE_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_wikiimage_trace_filepaths")
        elif tmp_workload_name == TraceLoader.TENCENTPHOTO1_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_tencentphoto1_trace_filepaths")
        elif tmp_workload_name == TraceLoader.TENCENTPHOTO2_WORKLOADNAME:
            tmp_filename_list = JsonUtil.getValueForKeystr(Common.scriptname, "trace_dirpath_relative_tencentphoto2_trace_filepaths")
        else:
            LogUtil.die(Common.scriptname, "unknown workload {}!".format(workload_name))

        # Load statistics (keys, value sizes, and per-key frequency) from trace files of the workload
        tmp_trace_loader = TraceLoader(Common.trace_dirpath, tmp_filename_list, tmp_workload_name)

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

        if cloud_machine_idx == Common.cur_machine_idx: # NOTE: clients and cloud may co-locate in the same physical machine
            # NOTE: NO need to check and copy, as current physical machine MUST have the characteristics file, which is just generated by the previous step in this script
            continue
        else: # Remote cloud machine
            # Check if cloud machine has characteristics file
            # NOTE: cloud machine may already have the characteristics file (e.g., trace files have been curvefitted before, or cloud machine is one of client machines yet not the first client)
            LogUtil.prompt(Common.scriptname, "check if characteristics file {} exists in cloud machine...".format(tmp_workload_characteristic_filepath))
            check_cloud_characteristic_filepath_remote_cmd = ExpUtil.getRemoteCmd(cloud_machine_idx, "ls {}".format(tmp_workload_characteristic_filepath))
            need_copy_characteristic_file = False
            check_cloud_characteristic_filepath_subprocess = SubprocessUtil.runCmd(check_cloud_characteristic_filepath_remote_cmd)
            if check_cloud_characteristic_filepath_subprocess.returncode != 0: # cloud characteristics file not found
                need_copy_characteristic_file = True
            # elif SubprocessUtil.getSubprocessOutputstr(check_cloud_characteristic_filepath_subprocess) == "": # cloud characteristics file not found (OBSOLETE: existing empty directory could return empty string)
            #     need_copy_characteristic_file = True
            else: # cloud characteristics file is found
                need_copy_characteristic_file = False

            # If cloud machine does NOT have the characteristics file, copy it to cloud machine
            if need_copy_characteristic_file:
                # Mkdir for trace dirpath if not exist
                LogUtil.prompt(Common.scriptname, "create trace directory {} if not exist in cloud machine...".format(Common.trace_dirpath))
                try_to_create_trace_dirpath_remote_cmd = ExpUtil.getRemoteCmd(cloud_machine_idx, "mkdir -p {}".format(Common.trace_dirpath))
                try_to_create_trace_dirpath_subprocess = SubprocessUtil.runCmd(try_to_create_trace_dirpath_remote_cmd)
                if try_to_create_trace_dirpath_subprocess.returncode != 0:
                    LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(try_to_create_trace_dirpath_subprocess))

                # Copy characteristics file to cloud machine
                LogUtil.prompt(Common.scriptname, "copy characteristics file {} to cloud machine...".format(tmp_workload_characteristic_filepath))
                cloud_machine_public_ip = physical_machines[cloud_machine_idx]["public_ipstr"]
                copy_characteristic_file_remote_cmd = "scp -i {0} {1} {2}@{3}:{1}".format(Common.sshkey_filepath, tmp_workload_characteristic_filepath, Common.username, cloud_machine_public_ip)
                copy_characteristic_file_subprocess = SubprocessUtil.runCmd(copy_characteristic_file_remote_cmd, is_capture_output=False) # Copy characteristics file may be time-consuming
                if copy_characteristic_file_subprocess.returncode != 0:
                    LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(copy_characteristic_file_subprocess))
    
    gc.collect() # Avoid memory overflow