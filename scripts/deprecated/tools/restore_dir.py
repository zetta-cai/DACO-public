#!/usr/bin/env python3

import os
import sys
import subprocess

from ..common import *

# (1) Config.json

# Update json file paths for cachebench workloads
default_facebook_config_filepath = "lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json"
current_facebook_config_filepath = "{}/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_facebook_config_filepath, current_facebook_config_filepath, "config.json")
print("")

# (2) Scripts

# Update project lib directory for CMAKE_PREFIX_PATH in cachelib
default_lib_dirpath = "/home/jzcai/projects/covered-private/lib"
PathUtil.restore_dir(Common.scriptname, default_lib_dirpath, Common.lib_dirpath, "scripts/cachelib/build-package.sh")
print("")

# (3) Makefiles

# Update CACHEBENCH_DIRPATH for cachebench
default_cachebench_dirpath = "lib/CacheLib"
current_cachebench_dirpath = "{}/CacheLib".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_cachebench_dirpath, current_cachebench_dirpath, "src/mk/lib/cachebench.mk")

# Update LFU_DIRPATH for LFU cache
default_lfu_dirpath = "lib/caches"
current_lfu_dirpath = "{}/caches".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_lfu_dirpath, current_lfu_dirpath, "src/mk/cache/lfu.mk")

# Update BOOST_DIRPATH for libboost
default_boost_dirpath = "lib/boost-1.81.0"
current_boost_dirpath = "{}/boost-1.81.0".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_boost_dirpath, current_boost_dirpath, "src/mk/lib/boost.mk")

# Update ROCKSDB_DIRPATH for rocksdb
default_rocksdb_dirpath = "lib/rocksdb-8.1.1"
current_rocksdb_dirpath = "{}/rocksdb-8.1.1".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_rocksdb_dirpath, current_rocksdb_dirpath, "src/mk/lib/rocksdb.mk")

# Update SMHASHER_DIRPATH for mmh3
default_smhasher_dirpath = "lib/smhasher"
current_smhasher_dirpath = "{}/smhasher".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_smhasher_dirpath, current_smhasher_dirpath, "src/mk/lib/smhasher.mk")

# Update TOMMYDS_DIRPATH for tommyds
default_tommyds_dirpath = "lib/tommyds"
current_tommyds_dirpath = "{}/tommyds".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_tommyds_dirpath, current_tommyds_dirpath, "src/mk/lib/tommyds.mk")

# Update LHD_DIRPATH for lhd
default_lhd_dirpath = "lib/lhd"
current_lhd_dirpath = "{}/lhd".format(Common.lib_dirpath)
PathUtil.restore_dir(Common.scriptname, default_lhd_dirpath, current_lhd_dirpath, "src/mk/cache/lhd.mk")