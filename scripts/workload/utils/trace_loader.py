#!/usr/bin/env python3
# trace_loader: load statistics (including key, value size, and frequency) from replayed trace files, which will be characterized by Zipfian distribution for geo-distributed evaluation later.

import numpy as np

from ...common import *

class TraceLoader:

    # For workloads without trace files
    PLACEHOLDER_FILEPATH = "placeholder"

    # For Wikipedia CDN traces
    TSV_DELIMITER = "\t" # Tab-separated values used by Wikipedia Text/Image trace files
    # For Wikipedia Text trace files
    ZIPF_WIKITEXT_WORKLOADNAME = "zipf_wikitext"
    ZETA_WIKITEXT_WORKLOADNAME = "zeta_wikitext"
    WIKITEXT_COLUMNCNT = 4
    WIKITEXT_KEY_COLUMNIDX = 1
    WIKITEXT_VALSIZE_COLUMNIDX = 2
    # For Wikipedia Image trace files
    ZIPF_WIKIIMAGE_WORKLOADNAME = "zipf_wikiimage"
    ZETA_WIKIIMAGE_WORKLOADNAME = "zeta_wikiimage"
    WIKIIMAGE_COLUMNCNT = 5
    WIKIIMAGE_KEY_COLUMNIDX = 1
    WIKIIMAGE_VALSIZE_COLUMNIDX = 3

    # For Tencent photo caching trace
    LOG_DELIMITER = " " # Space-separated values used by Tencent photo caching trace files
    ZIPF_TENCENTPHOTO1_WORKLOADNAME = "zipf_tencentphoto1"
    ZIPF_TENCENTPHOTO2_WORKLOADNAME = "zipf_tencentphoto2"
    ZETA_TENCENTPHOTO1_WORKLOADNAME = "zeta_tencentphoto1"
    ZETA_TENCENTPHOTO2_WORKLOADNAME = "zeta_tencentphoto2"
    TENCENTPHOTO_COLUMNCNT = 8
    TENCENTPHOTO_KEY_COLUMNIDX = 1
    TENCENTPHOTO_IMGFORMAT_COLUMNIDX = 2
    TENCENTPHOTO_IMGFORMAT_JPG = 0
    TENCENTPHOTO_IMGFORMAT_WEB = 5
    TENCENTPHOTO_VALSIZE_COLUMNIDX = 4

    # For Twitter KV traces (need optype ratios)
    SORT_DELIMITER = "," # Comma-separated values used by Twitter KV trace files
    ZIPF_TWITTERKV2_WORKLOADNAME = "zipf_twitterkv2"
    ZIPF_TWITTERKV4_WORKLOADNAME = "zipf_twitterkv4"
    TWITTERKV_COLUMNCNT = 7
    TWITTERKV_KEY_COLUMNIDX = 1
    TWITTERKV_KEYSIZE_COLUMNIDX = 2
    TWITTERKV_VALSIZE_COLUMNIDX = 3
    TWITTERKV_OPTYPE_COLUMNIDX = 5
    TWITTERKV_MAX_CONTENT_SIZE = 50 * 1000 * 1000 * 1000 # Limited loaded content size for memory limitation as 50G (316G for cluster2 and 277G for cluster4)

    # For Facebook photo caching (NO trace; statistics coming from the paper figures)
    ZIPF_FACEBOOKPHOTO_WORKLOADNAME = "zipf_fbphoto"

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

        # For workloads requiring optype ratios
        self.read_ratio_ = 1.0
        self.update_ratio_ = 0.0
        self.delete_ratio_ = 0.0
        self.insert_ratio_ = 0.0

        # Check dirpath
        if not os.path.exists(dirpath):
            LogUtil.die(Common.scriptname, "directory {} does not exist for workload {}!".format(dirpath, workload_name))

        # Load each file to update the self.statistics_ based on workload name
        for tmp_filename in filename_list:
            tmp_filepath = os.path.join(dirpath, tmp_filename)
            if tmp_filename != TraceLoader.PLACEHOLDER_FILEPATH and not os.path.exists(tmp_filepath):
                LogUtil.die(Common.scriptname, "file {} does not exist for workload {}!".format(tmp_filepath, workload_name))

            if workload_name == TraceLoader.ZETA_WIKITEXT_WORKLOADNAME or workload_name == TraceLoader.ZIPF_WIKITEXT_WORKLOADNAME:
                self.loadFileOfWikiText_(tmp_filepath)
            elif workload_name == TraceLoader.ZETA_WIKIIMAGE_WORKLOADNAME or workload_name == TraceLoader.ZIPF_WIKIIMAGE_WORKLOADNAME:
                self.loadFileOfWikiImage_(tmp_filepath)
            elif workload_name == TraceLoader.ZETA_TENCENTPHOTO1_WORKLOADNAME or workload_name == TraceLoader.ZETA_TENCENTPHOTO2_WORKLOADNAME or workload_name == TraceLoader.ZIPF_TENCENTPHOTO1_WORKLOADNAME or workload_name == TraceLoader.ZIPF_TENCENTPHOTO2_WORKLOADNAME:
                self.loadFileOfTencentPhoto_(tmp_filepath)
            elif workload_name == TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME or workload_name == TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME:
                self.loadFileOfTwitterKV_(tmp_filepath)
            elif workload_name == TraceLoader.ZIPF_FACEBOOKPHOTO_WORKLOADNAME:
                self.loadFileOfFacebookPhoto_(tmp_filepath) # Must be "placeholder" for Facebook photo caching due to NO trace
            else:
                LogUtil.die(Common.scriptname, "unknown workload {}!".format(workload_name))
    
    @staticmethod
    def needOptypeRatios(workload_name):
        return workload_name == TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME or workload_name == TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME

    # (OBSOLETE for facebook photo caching without traces) Get sorted frequency list (sorted in descending order of frequencies in order to get ranks)

    # Get rank-frequency map (sorted in the insertion order, i.e., ascending order of ranks or descending order of frequencies)
    # NOTE: rank (i.e., sorted index + 1) >= 1 for Zipfian distribution (no matter power low (alpha >= 0) or zeta (alpha > 1))
    # NOTE: we can NOT sample frequencies before sorting, as it will change the object ranks and hence NOT vs. the entire trace
    def getRankFrequencyMap(self):
        rank_frequency_map = {}
        if self.workload_name_ == TraceLoader.ZIPF_FACEBOOKPHOTO_WORKLOADNAME:
            pop_x = np.array([10**0, 10**1, 10**2, 10**3, 10**4, 10**5, 10**6]) # NOTE: rank >= 1 for Zipfian distribution (no matter power low (alpha >= 0) or zeta (alpha > 1))
            pop_y = np.array([10**6, 10**5, 10**4, 10**3.1, 10**2.2, 10**1.2, 10**0])
            for i in range(len(pop_x)):
                tmp_rank = pop_x[i]
                tmp_frequency = pop_y[i]
                rank_frequency_map[tmp_rank] = tmp_frequency
        else:
            frequency_list = []
            for tmp_key in self.statistics_:
                tmp_frequency = self.statistics_[tmp_key][1]
                frequency_list.append(tmp_frequency)
            LogUtil.prompt(Common.scriptname, "sorting frequency list for workload {}...".format(self.workload_name_))
            frequency_list.sort(reverse=True)
            frequency_list = np.array(frequency_list)
            LogUtil.dump(Common.scriptname, "total opcnt: {}".format(frequency_list.sum()))

            for i in range(len(frequency_list)):
                tmp_rank = i + 1
                tmp_frequency = frequency_list[i]
                rank_frequency_map[tmp_rank] = tmp_frequency

        return rank_frequency_map

    # Get key size histogram
    def getKeySizeHistogram(self):
        keysize_histogram = [0] * TraceLoader.KEYSIZE_HISTOGRAM_SIZE
        min_keysize = -1
        max_keysize = -1
        avg_keysize = 0
        for tmp_key in self.statistics_:
            if self.workload_name_ == TraceLoader.ZETA_WIKITEXT_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_WIKITEXT_WORKLOADNAME:
                tmp_keysize = 8 # Wikipedia Text uses bigint
            elif self.workload_name_ == TraceLoader.ZETA_WIKIIMAGE_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_WIKIIMAGE_WORKLOADNAME:
                tmp_keysize = 8 # Wikipedia Image uses bigint
            elif self.workload_name_ == TraceLoader.ZETA_TENCENTPHOTO1_WORKLOADNAME or self.workload_name_ == TraceLoader.ZETA_TENCENTPHOTO2_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_TENCENTPHOTO1_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_TENCENTPHOTO2_WORKLOADNAME:
                tmp_keysize = 20 # Tencent Photo uses 20B checksum
            elif self.workload_name_ == TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME:
                tmp_keysize = len(tmp_key) # String-type keys
            elif self.workload_name_ == TraceLoader.ZIPF_FACEBOOKPHOTO_WORKLOADNAME:
                tmp_keysize = 8 # Use bigint for Facebook photo caching
            else:
                tmp_keysize = len(tmp_key)
            
            if min_keysize == -1 or tmp_keysize < min_keysize:
                min_keysize = tmp_keysize
            if max_keysize == -1 or tmp_keysize > max_keysize:
                max_keysize = tmp_keysize
            avg_keysize += tmp_keysize

            # NOTE: make sure that src/workload/zeta_workload_wrapper.c also treats per bucket as 1B and the histogram ranges from 1B to 1024B
            if tmp_keysize < 1: # Empty key makes no sense
                LogUtil.die(Common.scriptname, "invalid key size {} for workload {}!".format(tmp_keysize, self.workload_name_))
            tmp_keysize_bktidx = tmp_keysize - 1

            if tmp_keysize_bktidx < TraceLoader.KEYSIZE_HISTOGRAM_SIZE: # 1B - 1024B
                keysize_histogram[tmp_keysize_bktidx] += 1
            else:
                keysize_histogram[TraceLoader.KEYSIZE_HISTOGRAM_SIZE - 1] += 1
        avg_keysize /= len(self.statistics_)
        LogUtil.dump(Common.scriptname, "keycnt: {}; keysize min/max/avg: {}/{}/{}".format(len(self.statistics_), min_keysize, max_keysize, avg_keysize))
        return keysize_histogram

    # Get value size histogram
    def getValueSizeHistogram(self):
        valsize_histogram = [0] * TraceLoader.VALSIZE_HISTOGRAM_SIZE
        min_valsize = -1
        max_valsize = -1
        avg_valsize = 0
        for tmp_key in self.statistics_:
            tmp_valsize = self.statistics_[tmp_key][0]

            if min_valsize == -1 or tmp_valsize < min_valsize:
                min_valsize = tmp_valsize
            if max_valsize == -1 or tmp_valsize > max_valsize:
                max_valsize = tmp_valsize
            avg_valsize += tmp_valsize
            
            # NOTE: make sure that src/workload/zeta_workload_wrapper.c also treats per bucket as 1KiB and the histogram ranges from 1KiB to 10240KiB
            if tmp_valsize < 1: # Treat all small values (including empty values) as 1KiB
                tmp_valsize_bktidx = 0
            else:
                tmp_valsize_bktidx = int((tmp_valsize - 1) / 1024) # Each bucket of value size histogram is 1KiB
            
            if tmp_valsize_bktidx < TraceLoader.VALSIZE_HISTOGRAM_SIZE: # 1KiB - 10240KiB
                valsize_histogram[tmp_valsize_bktidx] += 1
            else:
                valsize_histogram[TraceLoader.VALSIZE_HISTOGRAM_SIZE - 1] += 1
        avg_valsize /= len(self.statistics_)
        LogUtil.dump(Common.scriptname, "keycnt: {}; valsize min/max/avg: {}/{}/{}".format(len(self.statistics_), min_valsize, max_valsize, avg_valsize))
        return valsize_histogram
    
    # Get optype ratios for workloads requiring them
    def getReadRatio(self):
        return self.read_ratio_
    def getUpdateRatio(self):
        return self.update_ratio_
    def getDeleteRatio(self):
        return self.delete_ratio_
    def getInsertRatio(self):
        return self.insert_ratio_
    
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
    
    # Load Tencent Photo Caching trace files
    # Format: timestamp photoID(20B-checksum) imgFormat sizeSpecification size hitOrMiss terminalType responseTime(ms)
    def loadFileOfTencentPhoto_(self, filepath):
        LogUtil.prompt(Common.scriptname, "loading trace file {} for workload {}...".format(filepath, self.workload_name_))

        f = open(filepath, mode="r")
        while True:
            tmp_line = f.readline()
            if tmp_line == "": # End of file
                break
            
            # Process each line
            tmp_columns = tmp_line.strip().split(TraceLoader.LOG_DELIMITER)
            if len(tmp_columns) != TraceLoader.TENCENTPHOTO_COLUMNCNT:
                LogUtil.die(Common.scriptname, "invalid column count {} in trace file {} for workload {}!".format(len(tmp_columns), filepath, self.workload_name_))
            tmp_key = tmp_columns[TraceLoader.TENCENTPHOTO_KEY_COLUMNIDX]
            tmp_imgformat = int(tmp_columns[TraceLoader.TENCENTPHOTO_IMGFORMAT_COLUMNIDX])
            tmp_valsize = int(tmp_columns[TraceLoader.TENCENTPHOTO_VALSIZE_COLUMNIDX])

            if tmp_imgformat == TraceLoader.TENCENTPHOTO_IMGFORMAT_JPG or tmp_imgformat == TraceLoader.TENCENTPHOTO_IMGFORMAT_WEB:
                self.updateStatistics_(tmp_key, tmp_valsize)
        f.close()
        return
    
    # Load Twitter KV trace files
    # Format: timestamp, anonymized key, key size, value size, client id, operation, TTL
    def loadFileOfTwitterKV_(self, filepath):
        LogUtil.prompt(Common.scriptname, "loading trace file {} for workload {}...".format(filepath, self.workload_name_))

        # Optype statistics for workloads requiring them
        read_cnt = 0
        update_cnt = 0
        delete_cnt = 0
        insert_cnt = 0

        f = open(filepath, mode="r")
        content_size = 0
        while True:
            tmp_line = f.readline()
            if tmp_line == "": # End of file
                break
            
            # Check content size for Twitter KV workloads
            content_size += len(tmp_line)
            if self.workload_name_ == TraceLoader.ZIPF_TWITTERKV2_WORKLOADNAME or self.workload_name_ == TraceLoader.ZIPF_TWITTERKV4_WORKLOADNAME:
                if content_size >= TraceLoader.TWITTERKV_MAX_CONTENT_SIZE:
                    LogUtil.warn(Common.scriptname, "content size exceeds limit {} for workload {}!".format(TraceLoader.TWITTERKV_MAX_CONTENT_SIZE, self.workload_name_))
                    break
            
            # Process each line
            tmp_columns = tmp_line.strip().split(TraceLoader.SORT_DELIMITER)
            if len(tmp_columns) != TraceLoader.TWITTERKV_COLUMNCNT:
                LogUtil.die(Common.scriptname, "invalid column count {} in trace file {} for workload {}!".format(len(tmp_columns), filepath, self.workload_name_))
            tmp_key = tmp_columns[TraceLoader.TWITTERKV_KEY_COLUMNIDX]
            tmp_valsize = int(tmp_columns[TraceLoader.TWITTERKV_VALSIZE_COLUMNIDX])
            self.updateStatistics_(tmp_key, tmp_valsize)

            # Update optype statistics for workloads requiring them
            tmp_optype = tmp_columns[TraceLoader.TWITTERKV_OPTYPE_COLUMNIDX]
            if tmp_optype == "get":
                read_cnt += 1
            elif tmp_optype == "set":
                update_cnt += 1
            elif tmp_optype == "delete":
                delete_cnt += 1
            elif tmp_optype == "add":
                insert_cnt += 1
        
        # Update optype ratios for workloads requiring them
        total_cnt = read_cnt + update_cnt + delete_cnt + insert_cnt
        self.read_ratio_ = float(read_cnt) / float(total_cnt)
        self.update_ratio_ = float(update_cnt) / float(total_cnt)
        self.delete_ratio_ = float(delete_cnt) / float(total_cnt)
        self.insert_ratio_ = float(insert_cnt) / float(total_cnt)

    def loadFileOfFacebookPhoto_(self, filepath):
        if os.path.basename(filepath) != TraceLoader.PLACEHOLDER_FILEPATH:
            LogUtil.die(Common.scriptname, "invalid file path {} for workload {}!".format(filepath, self.workload_name_))
        
        LogUtil.prompt(Common.scriptname, "use statisics from paper figures for workload {}...".format(self.workload_name_))

        # Refer to scripts/workload/facebook_photocache.py (statistics come from the figures in original paper on Facebook photo caching)
        dataset_size = int(1.3*10**6) # 1.3M
        dataset_keys = np.array(range(1, dataset_size + 1))

        # Get dataset value sizes based on the paper figures
        dataset_value_sizes = np.zeros(dataset_size)
        samples = np.random.uniform(0, 999, size=dataset_size)
        for i in range(len(samples)):
            tmp_sample = samples[i]
            if tmp_sample < 10:
                dataset_value_sizes[i] = 1024
            elif tmp_sample < 20:
                dataset_value_sizes[i] = 1024 + (2048 - 1024) * (tmp_sample - 9) / (19 - 9)
            elif tmp_sample < 200:
                dataset_value_sizes[i] = 2048 + (4096 - 2048) * (tmp_sample - 19) / (199 - 19)
            elif tmp_sample < 300:
                dataset_value_sizes[i] = 4096 + (8192 - 4096) * (tmp_sample - 199) / (299 - 199)
            elif tmp_sample < 400:
                dataset_value_sizes[i] = 8192 + (16384 - 8192) * (tmp_sample - 299) / (399 - 299)
            elif tmp_sample < 450:
                dataset_value_sizes[i] = 16384 + (32768 - 16384) * (tmp_sample - 399) / (449 - 399)
            elif tmp_sample < 650:
                dataset_value_sizes[i] = 32768 + (65536 - 32768) * (tmp_sample - 449) / (649 - 449)
            elif tmp_sample < 950:
                dataset_value_sizes[i] = 65536 + (131072 - 65536) * (tmp_sample - 649) / (949 - 649)
            elif tmp_sample < 991:
                dataset_value_sizes[i] = 131072 + (262144 - 131072) * (tmp_sample - 949) / (990 - 949)
            elif tmp_sample < 994:
                dataset_value_sizes[i] = 262144 + (524288 - 262144) * (tmp_sample - 990) / (993 - 990)
            elif tmp_sample < 997:
                dataset_value_sizes[i] = 524288 + (1048576 - 524288) * (tmp_sample - 993) / (996 - 993)
            elif tmp_sample < 1000:
                dataset_value_sizes[i] = 1048576 + (2097152 - 1048576) * (tmp_sample - 996) / (999 - 996)
            else:
                LogUtil.die(Common.scriptname, "invalid tmp_sample {}".format(tmp_sample))
        
        # Update self.statistics_
        for i in range(dataset_size):
            tmp_key = dataset_keys[i]
            tmp_valsize = dataset_value_sizes[i]
            self.updateStatistics_(str(tmp_key), tmp_valsize) # NOTE: all freqs of keys MUST be 1, while they are NOT used

        return

    def updateStatistics_(self, key, valsize):
        if key not in self.statistics_:
            self.statistics_[key] = [valsize, 1] # Insert value size and initialize frequency as 1
        else:
            self.statistics_[key][1] += 1 # Increase frequency by 1
        return