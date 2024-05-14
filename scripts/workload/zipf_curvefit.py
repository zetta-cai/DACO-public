#!/usr/bin/env python3
# zipf_curvefit: extract statistical charactertistics (including Zipfian constant, key size distribution, and value size distribution) from replayed trace files for geo-distributed evaluation.

from ..common import *

class TraceLoader:
    # Open trace files based on dirpath + filename list
    # Load trace files based on workload name

    def __init__(self, dirpath, filename_list, workload_name):
        self.dirpath_ = dirpath
        self.filename_list_ = filename_list
        self.workload_name_ = workload_name

        if not os.path.exists(dirpath):
            LogUtil.die(Common.scriptname, "directory {} does not exist!".format(dirpath))