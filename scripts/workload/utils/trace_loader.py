#!/usr/bin/env python3
# trace_loader: load statistics (including key, value size, and frequency) from replayed trace files, which will be characterized by Zipfian distribution for geo-distributed evaluation later.

import numpy as np

from ...common import *

class TraceLoader:

    TSV_DELIMITER = "\t" # Tab-separated values used by Wikipedia Text/Image trace files
    # For Wikipedia Text trace files
    WIKITEXT_WORKLOADNAME = "zeta_wikitext"
    WIKITEXT_COLUMNCNT = 4
    WIKITEXT_KEY_COLUMNIDX = 1
    WIKITEXT_VALSIZE_COLUMNIDX = 2
    # For Wikipedia Image trace files
    WIKIIMAGE_WORKLOADNAME = "zeta_wikiimage"
    WIKIIMAGE_COLUMNCNT = 5
    WIKIIMAGE_KEY_COLUMNIDX = 1
    WIKIIMAGE_VALSIZE_COLUMNIDX = 3
    # For key size histogram (from 1B to 1KiB; each bucket is 1B)
    KEYSIZE_HISTOGRAM_SIZE = 1024
    # For value size histogram (from 1KiB to 10240KiB; each bucket is 1KiB)
    VALSIZE_HISTOGRAM_SIZE = 10240

    # Open trace files based on dirpath + filename list
    # Load trace files based on workload name
    def __init__(self, dirpath, filename_list, workload_name):
        self.dirpath_ = dirpath
        self.filename_list_ = filename_list
        self.workload_name_ = workload_name
        self.statistics_ = {} # key: [value size, frequency]

        # Check dirpath
        if not os.path.exists(dirpath):
            LogUtil.die(Common.scriptname, "directory {} does not exist for workload {}!".format(dirpath, workload_name))

        # Load each file to update the self.statistics_ based on workload name
        for tmp_filename in filename_list:
            tmp_filepath = os.path.join(dirpath, tmp_filename)
            if not os.path.exists(tmp_filepath):
                LogUtil.die(Common.scriptname, "file {} does not exist for workload {}!".format(tmp_filepath, workload_name))

            if workload_name == TraceLoader.WIKITEXT_WORKLOADNAME:
                self.loadFileOfWikiText_(tmp_filepath)
            elif workload_name == TraceLoader.WIKIIMAGE_WORKLOADNAME:
                self.loadFileOfWikiImage_(tmp_filepath)
            else:
                LogUtil.die(Common.scriptname, "unknown workload {}!".format(workload_name))

    # Get sorted frequency list (sorted in descending order of frequencies in order to get ranks)
    # NOTE: rank (i.e., sorted index + 1) >= 1 for Zipfian distribution (no matter power low (alpha >= 0) or zeta (alpha > 1))
    # NOTE: we can NOT sample frequencies before sorting, as it will change the object ranks and hence NOT vs. the entire trace
    def getSortedFrequencyList(self):
        frequency_list = []
        for tmp_key in self.statistics_:
            tmp_frequency = self.statistics_[tmp_key][1]
            frequency_list.append(tmp_frequency)
        LogUtil.prompt(Common.scriptname, "sorting frequency list for workload {}...".format(self.workload_name_))
        frequency_list.sort(reverse=True)
        return np.array(frequency_list)

    # Get key size histogram
    def getKeySizeHistogram(self):
        keysize_histogram = [0] * TraceLoader.KEYSIZE_HISTOGRAM_SIZE
        for tmp_key in self.statistics_:
            if self.workload_name_ == TraceLoader.WIKITEXT_WORKLOADNAME:
                tmp_keysize = 8 # Wikipedia Text uses bigint
            elif self.workload_name_ == TraceLoader.WIKIIMAGE_WORKLOADNAME:
                tmp_keysize = 8 # Wikipedia Image uses bigint
            else:
                tmp_keysize = len(tmp_key)

            # NOTE: make sure that src/workload/zeta_workload_wrapper.c also treats per bucket as 1B and the histogram ranges from 1B to 1024B
            if tmp_keysize < 1: # Empty key makes no sense
                LogUtil.die(Common.scriptname, "invalid key size {} for workload {}!".format(tmp_keysize, self.workload_name_))
            tmp_keysize_bktidx = tmp_keysize - 1

            if tmp_keysize_bktidx < TraceLoader.KEYSIZE_HISTOGRAM_SIZE: # 1B - 1024B
                keysize_histogram[tmp_keysize_bktidx] += 1
            else:
                keysize_histogram[TraceLoader.KEYSIZE_HISTOGRAM_SIZE - 1] += 1
        return keysize_histogram

    # Get value size histogram
    def getValueSizeHistogram(self):
        valsize_histogram = [0] * TraceLoader.VALSIZE_HISTOGRAM_SIZE
        for tmp_key in self.statistics_:
            tmp_valsize = self.statistics_[tmp_key][0]
            
            # NOTE: make sure that src/workload/zeta_workload_wrapper.c also treats per bucket as 1KiB and the histogram ranges from 1KiB to 10240KiB
            if tmp_valsize < 1: # Treat all small values (including empty values) as 1KiB
                tmp_valsize_bktidx = 0
            else:
                tmp_valsize_bktidx = int((tmp_valsize - 1) / 1024) # Each bucket of value size histogram is 1KiB
            
            if tmp_valsize_bktidx < TraceLoader.VALSIZE_HISTOGRAM_SIZE: # 1KiB - 10240KiB
                valsize_histogram[tmp_valsize_bktidx] += 1
            else:
                valsize_histogram[TraceLoader.VALSIZE_HISTOGRAM_SIZE - 1] += 1
        return valsize_histogram
    
    # Load Wikipedia Text trace files
    # Format: relative_unix,hashed_host_path_query,response_size,time_firstbyte
    def loadFileOfWikiText_(self, filepath):
        LogUtil.prompt(Common.scriptname, "loading trace file {} for workload {}...".format(filepath, self.workload_name_))

        is_first_line = True
        f = open(filepath, mode="r")
        while True:
            tmp_line = f.readline()
            if tmp_line == "": # End of file
                break
            if is_first_line: # Skip the first line (column headers)
                is_first_line = False
                continue

            # Process each non-first line
            tmp_columns = tmp_line.strip().split(TraceLoader.TSV_DELIMITER)
            if len(tmp_columns) != TraceLoader.WIKITEXT_COLUMNCNT:
                LogUtil.die(Common.scriptname, "invalid column count {} in trace file {} for workload {}!".format(len(tmp_columns), filepath, self.workload_name_))
            tmp_key = int(tmp_columns[TraceLoader.WIKITEXT_KEY_COLUMNIDX])
            tmp_valsize = int(tmp_columns[TraceLoader.WIKITEXT_VALSIZE_COLUMNIDX])
            self.updateStatistics_(tmp_key, tmp_valsize)
        f.close()
        return
    
    # Load Wikipedia Image trace files
    # Format: relative_unix,hashed_path_query,image_type,response_size,time_firstbyte
    def loadFileOfWikiImage_(self, filepath):
        LogUtil.prompt(Common.scriptname, "loading trace file {} for workload {}...".format(filepath, self.workload_name_))

        is_first_line = True
        f = open(filepath, mode="r")
        while True:
            tmp_line = f.readline()
            if tmp_line == "": # End of file
                break
            if is_first_line: # Skip the first line (column headers)
                is_first_line = False
                continue

            # Process each non-first line
            tmp_columns = tmp_line.strip().split(TraceLoader.TSV_DELIMITER)
            if len(tmp_columns) != TraceLoader.WIKIIMAGE_COLUMNCNT:
                LogUtil.die(Common.scriptname, "invalid column count {} in trace file {} for workload {}!".format(len(tmp_columns), filepath, self.workload_name_))
            tmp_key = int(tmp_columns[TraceLoader.WIKIIMAGE_KEY_COLUMNIDX])
            tmp_valsize = int(tmp_columns[TraceLoader.WIKIIMAGE_VALSIZE_COLUMNIDX])
            self.updateStatistics_(tmp_key, tmp_valsize)
        f.close()
        return

    def updateStatistics_(self, key, valsize):
        if key not in self.statistics_:
            self.statistics_[key] = [valsize, 1] # Insert value size and initialize frequency as 1
        else:
            self.statistics_[key][1] += 1 # Increase frequency by 1
        return