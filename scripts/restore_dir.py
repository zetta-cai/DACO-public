#!/usr/bin/env python3

import os
import sys
import subprocess

from common import *
from utils.util import *

def restore_dir(scriptname, original_dir, current_dir, path):
    if not os.path.exists(path):
        warn(scriptname, "Path does not exist: " + path)
        return
    
    restore_rootdir_cmd = "sed -i 's!{}!{}!g' {}".format(current_dir, original_dir, path)
    restore_rootdir_subprocess = subprocess.run(restore_rootdir_cmd, shell=True)
    if restore_rootdir_subprocess.returncode != 0:
        die(scriptname, "Failed to restore rootdir: " + path)

# (1) Config.json

# Update json file paths for cachebench workloads
default_facebook_config_filepath = "lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json"
current_facebook_config_filepath = "{}/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json".format(lib_dirpath)
restore_dir(scriptname, default_facebook_config_filepath, current_facebook_config_filepath, "config.json")

# (2) Scripts

# Update project lib directory for CMAKE_PREFIX_PATH in cachelib
default_lib_dirpath = "/home/sysheng/projects/covered-private/lib"
restore_dir(scriptname, default_lib_dirpath, lib_dirpath, "scripts/cachelib/build-package.sh")

# (3) Makefiles

# Update CACHEBENCH_DIRPATH for cachebench
default_cachebench_dirpath = "lib/CacheLib"
current_cachebench_dirpath = "{}/CacheLib".format(lib_dirpath)
restore_dir(scriptname, default_cachebench_dirpath, current_cachebench_dirpath, "src/mk/lib/cachebench.mk")

# Update LFU_DIRPATH for LFU cache
default_lfu_dirpath = "lib/caches"
current_lfu_dirpath = "{}/caches".format(lib_dirpath)
restore_dir(scriptname, default_lfu_dirpath, current_lfu_dirpath, "src/mk/cache/lfu.mk")

# Update BOOST_DIRPATH for libboost
default_boost_dirpath = "lib/boost_1_81_0"
current_boost_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
restore_dir(scriptname, default_boost_dirpath, current_boost_dirpath, "src/mk/lib/boost.mk")

# Update ROCKSDB_DIRPATH for rocksdb
default_rocksdb_dirpath = "lib/rocksdb-8.1.1"
current_rocksdb_dirpath = "{}/rocksdb-8.1.1".format(lib_dirpath)
restore_dir(scriptname, default_rocksdb_dirpath, current_rocksdb_dirpath, "src/mk/lib/rocksdb.mk")

# Update SMHASHER_DIRPATH for mmh3
default_smhasher_dirpath = "lib/smhasher"
current_smhasher_dirpath = "{}/smhasher".format(lib_dirpath)
restore_dir(scriptname, default_smhasher_dirpath, current_smhasher_dirpath, "src/mk/lib/smhasher.mk")

# Update TOMMYDS_DIRPATH for tommyds
default_tommyds_dirpath = "lib/tommyds"
current_tommyds_dirpath = "{}/tommyds".format(lib_dirpath)
restore_dir(scriptname, default_tommyds_dirpath, current_tommyds_dirpath, "src/mk/lib/tommyds.mk")