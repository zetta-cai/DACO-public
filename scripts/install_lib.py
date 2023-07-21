#!/usr/bin/env python3

import os
import sys
import subprocess

from paths import *

is_clear_tarball = False # whether to clear intermediate tarball files

# Include common module for the following installation
from common import *

# (1) Install boost 1.81.0

boost_download_filepath = "{}/boost_1_81_0.tar.gz".format(lib_dirpath)
if not os.path.exists(boost_download_filepath):
    prompt(filename, "download {}...".format(boost_download_filepath))
    boost_download_cmd = "cd {} && wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz".format(lib_dirpath)

    boost_download_subprocess = subprocess.run(boost_download_cmd, shell=True)
    if boost_download_subprocess.returncode != 0:
        die(filename, "failed to download {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists (boost has been downloaded)".format(boost_download_filepath))

boost_decompress_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
if not os.path.exists(boost_decompress_dirpath):
    prompt(filename, "decompress {}...".format(boost_download_filepath))
    boost_decompress_cmd = "cd {} && tar -xzvf boost_1_81_0.tar.gz".format(lib_dirpath)

    boost_decompress_subprocess = subprocess.run(boost_decompress_cmd, shell=True)
    if boost_decompress_subprocess.returncode != 0:
        die(filename, "failed to decompress {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists (boost has been decompressed)".format(boost_decompress_dirpath))

boost_install_dirpath = "{}/install".format(boost_decompress_dirpath)
if not os.path.exists(boost_install_dirpath):
    prompt(filename, "install libboost from source...")
    boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace --prefix=./install && sudo ./b2 install".format(boost_decompress_dirpath)
    # Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
    #boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test --prefix=./install && sudo ./b2 install && sudo ldconfig".format(boost_decompress_dirpath)

    boost_install_subprocess = subprocess.run(boost_install_cmd, shell=True)
    if boost_install_subprocess.returncode != 0:
        die(filename, "failed to install {}".format(boost_install_dirpath))
else:
    dump(filename, "{} exists (boost has been installed)".format(boost_install_dirpath))

if is_clear_tarball:
    warn(filename, "clear {}".format(boost_download_filepath))
    boost_clear_cmd = "cd {} && rm {}".format(lib_dirpath, boost_download_filepath)

    boost_clear_subprocess = subprocess.run(boost_clear_cmd, shell=True)
    if boost_clear_subprocess.returncode != 0:
        die(filename, "failed to clear {}".format(boost_download_filepath))

# (2) Install cachelib (commit ID: 3d475f6)

cachelib_clone_dirpath = "{}/CacheLib".format(lib_dirpath)
if not os.path.exists(cachelib_clone_dirpath):
    prompt(filename, "clone cachelib into {}...".format(cachelib_clone_dirpath))
    cachelib_clone_cmd = "cd {} && git clone https://github.com/facebook/CacheLib.git".format(lib_dirpath)

    cachelib_clone_subprocess = subprocess.run(cachelib_clone_cmd, shell=True)
    if cachelib_clone_subprocess.returncode != 0:
        die(filename, "failed to clone cachelib into {}".format(cachelib_clone_dirpath))
else:
    dump(filename, "{} exists (cachelib has been cloned)".format(cachelib_clone_dirpath))

cachelib_targetcommit = "7886d6d"
cachelib_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(cachelib_clone_dirpath)
cachelib_checkversion_subprocess = subprocess.run(cachelib_checkversion_cmd, shell=True, capture_output=True)
if cachelib_checkversion_subprocess.returncode != 0:
    die(filename, "failed to get the latest commit ID of cachelib")
else:
    cachelib_checkversion_outputbytes = cachelib_checkversion_subprocess.stdout
    cachelib_checkversion_outputstr = cachelib_checkversion_outputbytes.decode("utf-8")
    if cachelib_targetcommit not in cachelib_checkversion_outputstr:
        prompt(filename, "the latest commit ID of cachelib is {} -> reset cache lib to commit {}...".format(cachelib_checkversion_outputstr, cachelib_targetcommit))
        cachelib_reset_cmd = "cd {} && git reset --hard {}".format(cachelib_clone_dirpath, cachelib_targetcommit)
        
        cachelib_reset_subprocess = subprocess.run(cachelib_reset_cmd, shell=True)
        if cachelib_reset_subprocess.returncode != 0:
            die(filename, "failed to reset cachelib")
    else:
        dump(filename, "the latest commit ID of cachelib is already {}".format(cachelib_targetcommit))

# Note: cachelib will first build third-party libs and then itself libs
cachelib_cachebench_filepath = "{}/build-cachelib/cachebench/libcachelib_cachebench.a".format(cachelib_clone_dirpath)
cachelib_allocator_filepath = "{}/opt/cachelib/lib/libcachelib_allocator.a".format(cachelib_clone_dirpath)
if not os.path.exists(cachelib_cachebench_filepath) or not os.path.exists(cachelib_allocator_filepath):
    prompt(filename, "execute contrib/build.sh in {} to install cachelib...".format(cachelib_clone_dirpath))
    cachelib_install_cmd = "cd {} && ./contrib/build.sh -j -T".format(cachelib_clone_dirpath)

    cachelib_install_subprocess = subprocess.run(cachelib_install_cmd, shell=True)
    if cachelib_install_subprocess.returncode != 0:
        die(filename, "failed to install cachelib")
else:
    dump(filename, "cachelib has already been installed")

# (3) Install LRU cache (commit ID: de1c4a0)

lrucache_clone_dirpath = "{}/cpp-lru-cache".format(lib_dirpath)
if not os.path.exists(lrucache_clone_dirpath):
    prompt(filename, "clone LRU cache into {}...".format(lrucache_clone_dirpath))
    lrucache_clone_cmd = "cd {} && git clone https://github.com/lamerman/cpp-lru-cache.git".format(lib_dirpath)

    lrucache_clone_subprocess = subprocess.run(lrucache_clone_cmd, shell=True)
    if lrucache_clone_subprocess.returncode != 0:
        die(filename, "failed to clone LRU cache into {}".format(lrucache_clone_dirpath))
else:
    dump(filename, "{} exists (LRU cache has been cloned)".format(lrucache_clone_dirpath))

lrucache_targetcommit = "de1c4a0"
lrucache_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(lrucache_clone_dirpath)
lrucache_checkversion_subprocess = subprocess.run(lrucache_checkversion_cmd, shell=True, capture_output=True)
if lrucache_checkversion_subprocess.returncode != 0:
    die(filename, "failed to get the latest commit ID of LRU cache")
else:
    lrucache_checkversion_outputbytes = lrucache_checkversion_subprocess.stdout
    lrucache_checkversion_outputstr = lrucache_checkversion_outputbytes.decode("utf-8")
    if lrucache_targetcommit not in lrucache_checkversion_outputstr:
        prompt(filename, "the latest commit ID of LRU cache is {} -> reset LRU cache to commit {}...".format(lrucache_checkversion_outputstr, lrucache_targetcommit))
        lrucache_reset_cmd = "cd {} && git reset --hard {}".format(lrucache_clone_dirpath, lrucache_targetcommit)
        
        lrucache_checkversion_subprocess = subprocess.run(lrucache_checkversion_cmd, shell=True)
        if lrucache_checkversion_subprocess.returncode != 0:
            die(filename, "failed to reset LRU cache")
    else:
        dump(filename, "the latest commit ID of LRU cache is already {}".format(lrucache_targetcommit))

# (4) Install RocksDB 8.1.1

rocksdb_download_filepath="{}/rocksdb-8.1.1.tar.gz".format(lib_dirpath)
if not os.path.exists(rocksdb_download_filepath):
    prompt(filename, "download {}...".format(rocksdb_download_filepath))
    # NOTE: github url needs content-disposition to be converted into the the real url
    rocksdb_download_cmd = "cd {} && wget --content-disposition https://github.com/facebook/rocksdb/archive/refs/tags/v8.1.1.tar.gz".format(lib_dirpath)

    rocksdb_download_subprocess = subprocess.run(rocksdb_download_cmd, shell=True)
    if rocksdb_download_subprocess.returncode != 0:
        die(filename, "failed to download {}".format(rocksdb_download_filepath))
else:
    dump(filename, "{} exists (rocksdb has been downloaded)".format(rocksdb_download_filepath))

rocksdb_decompress_dirpath = "{}/rocksdb-8.1.1".format(lib_dirpath)
if not os.path.exists(rocksdb_decompress_dirpath):
    prompt(filename, "decompress {}...".format(rocksdb_download_filepath))
    rocksdb_decompress_cmd = "cd {} && tar -xzvf rocksdb-8.1.1.tar.gz".format(lib_dirpath)

    rocksdb_decompress_subprocess = subprocess.run(rocksdb_decompress_cmd, shell=True)
    if rocksdb_decompress_subprocess.returncode != 0:
        die(filename, "failed to decompress {}".format(rocksdb_download_filepath))
else:
    dump(filename, "{} exists (rocksdb has been decompressed)".format(rocksdb_decompress_dirpath))

rocksdb_install_dirpath = "{}/librocksdb.a".format(rocksdb_decompress_dirpath)
if not os.path.exists(rocksdb_install_dirpath):
    prompt(filename, "install librocksdb.a from source...")
    rocksdb_install_cmd = "sudo apt-get install libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libjemalloc-dev libsnappy-dev && cd {} && PORTABLE=1 make static_lib".format(rocksdb_decompress_dirpath)

    rocksdb_install_subprocess = subprocess.run(rocksdb_install_cmd, shell=True)
    if rocksdb_install_subprocess.returncode != 0:
        die(filename, "failed to install {}".format(rocksdb_install_dirpath))
else:
    dump(filename, "{} exists (rocksdb has been installed)".format(rocksdb_install_dirpath))

if is_clear_tarball:
    warn(filename, "clear {}".format(rocksdb_download_filepath))
    rocksdb_clear_cmd = "cd {} && rm {}".format(rocksdb_download_filepath).format(lib_dirpath)

    rocksdb_clear_subprocess = subprocess.run(rocksdb_clear_cmd, shell=True)
    if rocksdb_clear_subprocess.returncode != 0:
        die(filename, "failed to clear {}".format(rocksdb_download_filepath))

# (5) Install SMHasher (commit ID: 61a0530)

smhasher_clone_dirpath = "{}/smhasher".format(lib_dirpath)
if not os.path.exists(smhasher_clone_dirpath):
    prompt(filename, "clone SMHasher into {}...".format(smhasher_clone_dirpath))
    smhasher_clone_cmd = "cd {} && git clone https://github.com/aappleby/smhasher.git".format(lib_dirpath)

    smhasher_clone_subprocess = subprocess.run(smhasher_clone_cmd, shell=True)
    if smhasher_clone_subprocess.returncode != 0:
        die(filename, "failed to clone SMHasher into {}".format(smhasher_clone_dirpath))
else:
    dump(filename, "{} exists (SMHasher has been cloned)".format(smhasher_clone_dirpath))

smhasher_targetcommit = "61a0530"
smhahser_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(smhasher_clone_dirpath)
smhasher_checkversion_subprocess = subprocess.run(smhahser_checkversion_cmd, shell=True, capture_output=True)
if smhasher_checkversion_subprocess.returncode != 0:
    die(filename, "failed to get the latest commit ID of SMHasher")
else:
    smhasher_checkversion_outputbytes = smhasher_checkversion_subprocess.stdout
    smhasher_checkversion_outputstr = smhasher_checkversion_outputbytes.decode("utf-8")
    if smhasher_targetcommit not in smhasher_checkversion_outputstr:
        prompt(filename, "the latest commit ID of SMHasher is {} -> reset SMHasher to commit {}...".format(smhasher_checkversion_outputstr, smhasher_targetcommit))
        smhasher_reset_cmd = "cd {} && git reset --hard {}".format(smhasher_clone_dirpath, smhasher_targetcommit)
        
        smhasher_checkversion_subprocess = subprocess.run(smhasher_reset_cmd, shell=True)
        if smhasher_checkversion_subprocess.returncode != 0:
            die(filename, "failed to reset SMHasher")
    else:
        dump(filename, "the latest commit ID of SMHasher is already {}".format(smhasher_targetcommit))

# (6) Chown of libraries

prompt(filename, "chown of libraries...")
chown_cmd = "sudo chown -R ${USER}:${USER} {}".format(lib_dirpath)
chown_subprocess = subprocess.run(chown_cmd, shell=True)
if chown_subprocess.returncode != 0:
    die(filename, "failed to chown of libraries")