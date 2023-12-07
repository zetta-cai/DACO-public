#!/usr/bin/env python3

import os
import sys
import subprocess

from common import *
from utils.util import *

def replace_dir(scriptname, original_dir, current_dir, path):
    if not os.path.exists(path):
        warn(scriptname, "Path does not exist: " + path)
        return
    
    replace_rootdir_cmd = "sed -i 's!{}!{}!g' {}".format(original_dir, current_dir, path)
    replace_rootdir_subprocess = runCmd(replace_rootdir_cmd)
    if replace_rootdir_subprocess.returncode != 0:
        die(scriptname, "Failed to replace rootdir: " + path)

# (1) Config.json

# Update json file paths for cachebench workloads
default_facebook_config_filepath = "lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json"
current_facebook_config_filepath = "{}/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json".format(lib_dirpath)
replace_dir(scriptname, default_facebook_config_filepath, current_facebook_config_filepath, "config.json")
print("")

# (2) Scripts

# Update project lib directory for CMAKE_PREFIX_PATH in cachelib
default_lib_dirpath = "/home/sysheng/projects/covered-private/lib"
replace_dir(scriptname, default_lib_dirpath, lib_dirpath, "scripts/cachelib/build-package.sh")
print("")

# (3) Makefiles

# Update CACHEBENCH_DIRPATH for cachebench
default_cachebench_dirpath = "lib/CacheLib"
current_cachebench_dirpath = "{}/CacheLib".format(lib_dirpath)
replace_dir(scriptname, default_cachebench_dirpath, current_cachebench_dirpath, "src/mk/lib/cachebench.mk")
print("")

# Update LFU_DIRPATH for LFU cache
default_lfu_dirpath = "lib/caches"
current_lfu_dirpath = "{}/caches".format(lib_dirpath)
replace_dir(scriptname, default_lfu_dirpath, current_lfu_dirpath, "src/mk/cache/lfu.mk")
print("")

# Update BOOST_DIRPATH for libboost
default_boost_dirpath = "lib/boost_1_81_0"
current_boost_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
replace_dir(scriptname, default_boost_dirpath, current_boost_dirpath, "src/mk/lib/boost.mk")
print("")

# Update ROCKSDB_DIRPATH for rocksdb
default_rocksdb_dirpath = "lib/rocksdb-8.1.1"
current_rocksdb_dirpath = "{}/rocksdb-8.1.1".format(lib_dirpath)
replace_dir(scriptname, default_rocksdb_dirpath, current_rocksdb_dirpath, "src/mk/lib/rocksdb.mk")
print("")

# Update SMHASHER_DIRPATH for mmh3
default_smhasher_dirpath = "lib/smhasher"
current_smhasher_dirpath = "{}/smhasher".format(lib_dirpath)
replace_dir(scriptname, default_smhasher_dirpath, current_smhasher_dirpath, "src/mk/lib/smhasher.mk")
print("")

# Update TOMMYDS_DIRPATH for tommyds
default_tommyds_dirpath = "lib/tommyds"
current_tommyds_dirpath = "{}/tommyds".format(lib_dirpath)
replace_dir(scriptname, default_tommyds_dirpath, current_tommyds_dirpath, "src/mk/lib/tommyds.mk")
print("")

# Update LHD_DIRPATH for lhd
default_lhd_dirpath = "lib/lhd"
current_lhd_dirpath = "{}/lhd".format(lib_dirpath)
replace_dir(scriptname, default_lhd_dirpath, current_lhd_dirpath, "src/mk/cache/lhd.mk")