#!/usr/bin/env python3

import os
import sys

from common import *

is_clear_tarball = False # whether to clear intermediate tarball files

# Variables to control whether to install the corresponding softwares
is_install_boost = False
is_install_cachelib = False
is_install_lrucache = False
is_install_lfucache = False
is_install_rocksdb = False
is_install_smhasher = False
is_install_segcache = False

# Include util module for the following installation
from util import *

# (0) Check input CLI parameters

if len(sys.argv) != 2:
    die(filename, "Usage: sudo python3 scripts/install_lib.py $LD_LIBRARY_PATH")

# (1) Install boost 1.81.0

if is_install_boost:
    boost_download_filepath = "{}/boost_1_81_0.tar.gz".format(lib_dirpath)
    if not os.path.exists(boost_download_filepath):
        prompt(filename, "download {}...".format(boost_download_filepath))
        boost_download_cmd = "cd {} && wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz".format(lib_dirpath)

        boost_download_subprocess = runCmd(boost_download_cmd)
        if boost_download_subprocess.returncode != 0:
            boost_download_errstr = getSubprocessErrstr(boost_download_subprocess)
            die(filename, "failed to download {}; error: {}".format(boost_download_filepath, boost_download_errstr))
    else:
        dump(filename, "{} exists (boost has been downloaded)".format(boost_download_filepath))

    boost_decompress_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
    if not os.path.exists(boost_decompress_dirpath):
        prompt(filename, "decompress {}...".format(boost_download_filepath))
        boost_decompress_cmd = "cd {} && tar -xzvf boost_1_81_0.tar.gz".format(lib_dirpath)

        boost_decompress_subprocess = runCmd(boost_decompress_cmd)
        if boost_decompress_subprocess.returncode != 0:
            boost_decompress_errstr = getSubprocessErrstr(boost_decompress_subprocess)
            die(filename, "failed to decompress {}; error: {}".format(boost_download_filepath, boost_decompress_errstr))
    else:
        dump(filename, "{} exists (boost has been decompressed)".format(boost_decompress_dirpath))

    boost_install_dirpath = "{}/install".format(boost_decompress_dirpath)
    if not os.path.exists(boost_install_dirpath):
        prompt(filename, "install libboost from source...")
        boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=./install && sudo ./b2 install".format(boost_decompress_dirpath)
        # Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
        #boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=./install && sudo ./b2 install && sudo ldconfig".format(boost_decompress_dirpath)

        boost_install_subprocess = runCmd(boost_install_cmd)
        if boost_install_subprocess.returncode != 0:
            boost_install_errstr = getSubprocessErrstr(boost_install_subprocess)
            die(filename, "failed to install {}; error: {}".format(boost_install_dirpath, boost_install_errstr))
    else:
        dump(filename, "{} exists (boost has been installed)".format(boost_install_dirpath))

# (2) Install cachelib (commit ID: 3d475f6)

if is_install_cachelib:
    cachelib_clone_dirpath = "{}/CacheLib".format(lib_dirpath)
    if not os.path.exists(cachelib_clone_dirpath):
        prompt(filename, "clone cachelib into {}...".format(cachelib_clone_dirpath))
        cachelib_clone_cmd = "cd {} && git clone https://github.com/facebook/CacheLib.git".format(lib_dirpath)

        cachelib_clone_subprocess = runCmd(cachelib_clone_cmd)
        if cachelib_clone_subprocess.returncode != 0:
            cachelib_clone_errstr = getSubprocessErrstr(cachelib_clone_subprocess)
            die(filename, "failed to clone cachelib into {}; error: {}".format(cachelib_clone_dirpath, cachelib_clone_errstr))
    else:
        dump(filename, "{} exists (cachelib has been cloned)".format(cachelib_clone_dirpath))

    cachelib_targetcommit = "7886d6d"
    cachelib_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(cachelib_clone_dirpath)
    cachelib_checkversion_subprocess = runCmd(cachelib_checkversion_cmd)
    if cachelib_checkversion_subprocess.returncode != 0:
        cachelib_checkversion_errstr = getSubprocessErrstr(cachelib_checkversion_subprocess)
        die(filename, "failed to get the latest commit ID of cachelib; error: {}".format(cachelib_checkversion_errstr))
    else:
        cachelib_checkversion_outputstr = getSubprocessOutputstr(cachelib_checkversion_subprocess)
        if cachelib_targetcommit not in cachelib_checkversion_outputstr:
            prompt(filename, "the latest commit ID of cachelib is {} -> reset cache lib to commit {}...".format(cachelib_checkversion_outputstr, cachelib_targetcommit))
            cachelib_reset_cmd = "cd {} && git reset --hard {}".format(cachelib_clone_dirpath, cachelib_targetcommit)
            
            cachelib_reset_subprocess = runCmd(cachelib_reset_cmd)
            if cachelib_reset_subprocess.returncode != 0:
                cachelib_reset_errstr = getSubprocessErrstr(cachelib_reset_subprocess)
                die(filename, "failed to reset cachelib; error: {}".format(cachelib_reset_errstr))
        else:
            dump(filename, "the latest commit ID of cachelib is already {}".format(cachelib_targetcommit))

    # Note: cachelib will first build third-party libs and then itself libs
    cachelib_cachebench_filepath = "{}/build-cachelib/cachebench/libcachelib_cachebench.a".format(cachelib_clone_dirpath)
    cachelib_allocator_filepath = "{}/opt/cachelib/lib/libcachelib_allocator.a".format(cachelib_clone_dirpath)
    if not os.path.exists(cachelib_cachebench_filepath) or not os.path.exists(cachelib_allocator_filepath):
        # Update folly to use libboost 1.81.0
        prompt(filename, "replace contrib/build-package.sh to use libboost 1.81.0...")
        replace_build_package_cmd = "sudo cp scripts/cachelib/build-package.sh {}/contrib/build-package.sh".format(cachelib_clone_dirpath)
        replace_build_package_subprocess = runCmd(replace_build_package_cmd)
        if replace_build_package_subprocess.returncode != 0:
            replace_build_package_errstr = getSubprocessErrstr(replace_build_package_subprocess)
            die(filename, "failed to replace contrib/build-package.sh; error: {}".format(replace_build_package_errstr))

        # Build cachelib and its dependencies
        prompt(filename, "execute contrib/build.sh in {} to install cachelib...".format(cachelib_clone_dirpath))
        #cachelib_install_cmd = "cd {} && ./contrib/build.sh -j -T".format(cachelib_clone_dirpath)
        cachelib_install_cmd = "cd {} && ./contrib/build.sh -j -T -v -S".format(cachelib_clone_dirpath) # For debugging

        cachelib_install_subprocess = runCmd(cachelib_install_cmd)
        if cachelib_install_subprocess.returncode != 0:
            cachelib_install_errstr = getSubprocessErrstr(cachelib_install_subprocess)
            die(filename, "failed to install cachelib; error: {}".format(cachelib_install_errstr))
    else:
        dump(filename, "cachelib has already been installed")

# (3) Install LRU cache (commit ID: de1c4a0)

if is_install_lrucache:
    lrucache_clone_dirpath = "{}/cpp-lru-cache".format(lib_dirpath)
    if not os.path.exists(lrucache_clone_dirpath):
        prompt(filename, "clone LRU cache into {}...".format(lrucache_clone_dirpath))
        lrucache_clone_cmd = "cd {} && git clone https://github.com/lamerman/cpp-lru-cache.git".format(lib_dirpath)

        lrucache_clone_subprocess = runCmd(lrucache_clone_cmd)
        if lrucache_clone_subprocess.returncode != 0:
            lrucache_clone_errstr = getSubprocessErrstr(lrucache_clone_subprocess)
            die(filename, "failed to clone LRU cache into {}; error: {}".format(lrucache_clone_dirpath, lrucache_clone_errstr))
    else:
        dump(filename, "{} exists (LRU cache has been cloned)".format(lrucache_clone_dirpath))

    lrucache_targetcommit = "de1c4a0"
    lrucache_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(lrucache_clone_dirpath)
    lrucache_checkversion_subprocess = runCmd(lrucache_checkversion_cmd)
    if lrucache_checkversion_subprocess.returncode != 0:
        lrucache_checkversion_errstr = getSubprocessErrstr(lrucache_checkversion_subprocess)
        die(filename, "failed to get the latest commit ID of LRU cache; error: {}".format(lrucache_checkversion_errstr))
    else:
        lrucache_checkversion_outputstr = getSubprocessOutputstr(lrucache_checkversion_subprocess)
        if lrucache_targetcommit not in lrucache_checkversion_outputstr:
            prompt(filename, "the latest commit ID of LRU cache is {} -> reset LRU cache to commit {}...".format(lrucache_checkversion_outputstr, lrucache_targetcommit))
            lrucache_reset_cmd = "cd {} && git reset --hard {}".format(lrucache_clone_dirpath, lrucache_targetcommit)
            
            lrucache_reset_subprocess = runCmd(lrucache_reset_cmd)
            if lrucache_reset_subprocess.returncode != 0:
                lrucache_reset_errstr = getSubprocessErrstr(lrucache_reset_subprocess)
                die(filename, "failed to reset LRU cache; error: {}".format(lrucache_reset_errstr))
        else:
            dump(filename, "the latest commit ID of LRU cache is already {}".format(lrucache_targetcommit))

# (4) Install LFU cache (commit ID: 0f65db1)

if is_install_lfucache:
    lfucache_clone_dirpath = "{}/caches".format(lib_dirpath)
    if not os.path.exists(lfucache_clone_dirpath):
        prompt(filename, "clone LFU cache into {}...".format(lfucache_clone_dirpath))
        lfucache_clone_cmd = "cd {} && git clone https://github.com/vpetrigo/caches.git".format(lib_dirpath)

        lfucache_clone_subprocess = runCmd(lfucache_clone_cmd)
        if lfucache_clone_subprocess.returncode != 0:
            lfucache_clone_errstr = getSubprocessErrstr(lfucache_clone_subprocess)
            die(filename, "failed to clone LFU cache into {}; error: {}".format(lfucache_clone_dirpath, lfucache_clone_errstr))
    else:
        dump(filename, "{} exists (LFU cache has been cloned)".format(lfucache_clone_dirpath))

    lfucache_targetcommit = "0f65db1"
    lfucache_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(lfucache_clone_dirpath)
    lfucache_checkversion_subprocess = runCmd(lfucache_checkversion_cmd)
    if lfucache_checkversion_subprocess.returncode != 0:
        lfucache_checkversion_errstr = getSubprocessErrstr(lfucache_checkversion_subprocess)
        die(filename, "failed to get the latest commit ID of LFU cache; error: {}".format(lfucache_checkversion_errstr))
    else:
        lfucache_checkversion_outputstr = getSubprocessOutputstr(lfucache_checkversion_subprocess)
        if lfucache_targetcommit not in lfucache_checkversion_outputstr:
            prompt(filename, "the latest commit ID of LFU cache is {} -> reset LFU cache to commit {}...".format(lfucache_checkversion_outputstr, lfucache_targetcommit))
            lfucache_reset_cmd = "cd {} && git reset --hard {}".format(lfucache_clone_dirpath, lfucache_targetcommit)
            
            lfucache_reset_subprocess = runCmd(lfucache_reset_cmd)
            if lfucache_reset_subprocess.returncode != 0:
                lfucache_reset_errstr = getSubprocessErrstr(lfucache_reset_subprocess)
                die(filename, "failed to reset LFU cache; error: {}".format(lfucache_reset_errstr))
        else:
            dump(filename, "the latest commit ID of LFU cache is already {}".format(lfucache_targetcommit))

# (5) Install RocksDB 8.1.1

if is_install_rocksdb:
    rocksdb_download_filepath="{}/rocksdb-8.1.1.tar.gz".format(lib_dirpath)
    if not os.path.exists(rocksdb_download_filepath):
        prompt(filename, "download {}...".format(rocksdb_download_filepath))
        # NOTE: github url needs content-disposition to be converted into the the real url
        rocksdb_download_cmd = "cd {} && wget --content-disposition https://github.com/facebook/rocksdb/archive/refs/tags/v8.1.1.tar.gz".format(lib_dirpath)

        rocksdb_download_subprocess = runCmd(rocksdb_download_cmd)
        if rocksdb_download_subprocess.returncode != 0:
            rocksdb_download_errstr = getSubprocessErrstr(rocksdb_download_subprocess)
            die(filename, "failed to download {}; error: {}".format(rocksdb_download_filepath, rocksdb_download_errstr))
    else:
        dump(filename, "{} exists (rocksdb has been downloaded)".format(rocksdb_download_filepath))

    rocksdb_decompress_dirpath = "{}/rocksdb-8.1.1".format(lib_dirpath)
    if not os.path.exists(rocksdb_decompress_dirpath):
        prompt(filename, "decompress {}...".format(rocksdb_download_filepath))
        rocksdb_decompress_cmd = "cd {} && tar -xzvf rocksdb-8.1.1.tar.gz".format(lib_dirpath)

        rocksdb_decompress_subprocess = runCmd(rocksdb_decompress_cmd)
        if rocksdb_decompress_subprocess.returncode != 0:
            rocksdb_decompress_errstr = getSubprocessErrstr(rocksdb_decompress_subprocess)
            die(filename, "failed to decompress {}; error: {}".format(rocksdb_download_filepath, rocksdb_decompress_errstr))
    else:
        dump(filename, "{} exists (rocksdb has been decompressed)".format(rocksdb_decompress_dirpath))

    rocksdb_install_dirpath = "{}/librocksdb.a".format(rocksdb_decompress_dirpath)
    if not os.path.exists(rocksdb_install_dirpath):
        prompt(filename, "install librocksdb.a from source...")
        rocksdb_install_cmd = "sudo apt-get install libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libjemalloc-dev libsnappy-dev && cd {} && PORTABLE=1 make static_lib".format(rocksdb_decompress_dirpath)

        rocksdb_install_subprocess = runCmd(rocksdb_install_cmd)
        if rocksdb_install_subprocess.returncode != 0:
            rocksdb_install_errstr = getSubprocessErrstr(rocksdb_install_subprocess)
            die(filename, "failed to install {}; error: {}".format(rocksdb_install_dirpath, rocksdb_install_errstr))
    else:
        dump(filename, "{} exists (rocksdb has been installed)".format(rocksdb_install_dirpath))

    if is_clear_tarball:
        warn(filename, "clear {}".format(rocksdb_download_filepath))
        rocksdb_clear_cmd = "cd {} && rm {}".format(rocksdb_download_filepath).format(lib_dirpath)

        rocksdb_clear_subprocess = runCmd(rocksdb_clear_cmd)
        if rocksdb_clear_subprocess.returncode != 0:
            rocksdb_clear_errstr = getSubprocessErrstr(rocksdb_clear_subprocess)
            die(filename, "failed to clear {}; error: {}".format(rocksdb_download_filepath, rocksdb_clear_errstr))

# (6) Install SMHasher (commit ID: 61a0530)

if is_install_smhasher:
    smhasher_clone_dirpath = "{}/smhasher".format(lib_dirpath)
    if not os.path.exists(smhasher_clone_dirpath):
        prompt(filename, "clone SMHasher into {}...".format(smhasher_clone_dirpath))
        smhasher_clone_cmd = "cd {} && git clone https://github.com/aappleby/smhasher.git".format(lib_dirpath)

        smhasher_clone_subprocess = runCmd(smhasher_clone_cmd)
        if smhasher_clone_subprocess.returncode != 0:
            smhasher_clone_errstr = getSubprocessErrstr(smhasher_clone_subprocess)
            die(filename, "failed to clone SMHasher into {}; error: {}".format(smhasher_clone_dirpath, smhasher_clone_errstr))
    else:
        dump(filename, "{} exists (SMHasher has been cloned)".format(smhasher_clone_dirpath))

    smhasher_targetcommit = "61a0530"
    smhahser_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(smhasher_clone_dirpath)
    smhasher_checkversion_subprocess = runCmd(smhahser_checkversion_cmd)
    if smhasher_checkversion_subprocess.returncode != 0:
        smhasher_checkversion_errstr = getSubprocessErrstr(smhasher_checkversion_subprocess)
        die(filename, "failed to get the latest commit ID of SMHasher; error: {}".format(smhasher_checkversion_errstr))
    else:
        smhasher_checkversion_outputstr = getSubprocessOutputstr(smhasher_checkversion_subprocess)
        if smhasher_targetcommit not in smhasher_checkversion_outputstr:
            prompt(filename, "the latest commit ID of SMHasher is {} -> reset SMHasher to commit {}...".format(smhasher_checkversion_outputstr, smhasher_targetcommit))
            smhasher_reset_cmd = "cd {} && git reset --hard {}".format(smhasher_clone_dirpath, smhasher_targetcommit)
            
            smhasher_reset_subprocess = runCmd(smhasher_reset_cmd)
            if smhasher_reset_subprocess.returncode != 0:
                smhasher_reset_errstr = getSubprocessErrstr(smhasher_reset_subprocess)
                die(filename, "failed to reset SMHasher; error: {}".format(smhasher_reset_errstr))
        else:
            dump(filename, "the latest commit ID of SMHasher is already {}".format(smhasher_targetcommit))

# (7) Install SegCache (commit ID: 0abdfee)

# NOTE: we just use SegCache downloaded from github in lib/segcache as a reference, while always use our hacked version in src/cache/segcache to fix SegCache's implementation issues
if is_install_segcache:
    segcache_clone_dirpath = "{}/segcache".format(lib_dirpath)
    if not os.path.exists(segcache_clone_dirpath):
        prompt(filename, "clone SegCache into {}...".format(segcache_clone_dirpath))
        segcache_clone_cmd = "cd {} && git clone https://github.com/thesys-lab/segcache".format(lib_dirpath)

        segcache_clone_subprocess = runCmd(segcache_clone_cmd)
        if segcache_clone_subprocess.returncode != 0:
            segcache_clone_errstr = getSubprocessErrstr(segcache_clone_subprocess)
            die(filename, "failed to clone SegCache into {}; error: {}".format(segcache_clone_dirpath, segcache_clone_errstr))
    else:
        dump(filename, "{} exists (SegCache has been cloned)".format(segcache_clone_dirpath))

    segcache_targetcommit = "0abdfee"
    segcache_checkversion_cmd = "cd {} && git log --format=\"%H\" -n 1".format(segcache_clone_dirpath)
    segcache_checkversion_subprocess = runCmd(segcache_checkversion_cmd)
    if segcache_checkversion_subprocess.returncode != 0:
        segcache_checkversion_errstr = getSubprocessErrstr(segcache_checkversion_subprocess)
        die(filename, "failed to get the latest commit ID of SegCache; error: {}".format(segcache_checkversion_errstr))
    else:
        segcache_checkversion_outputstr = getSubprocessOutputstr(segcache_checkversion_subprocess)
        if segcache_targetcommit not in segcache_checkversion_outputstr:
            prompt(filename, "the latest commit ID of SegCache is {} -> reset SegCache to commit {}...".format(segcache_checkversion_outputstr, segcache_targetcommit))
            segcache_reset_cmd = "cd {} && git reset --hard {}".format(segcache_clone_dirpath, segcache_targetcommit)
            
            segcache_reset_subprocess = runCmd(segcache_reset_cmd)
            if segcache_reset_subprocess.returncode != 0:
                segcache_reset_errstr = getSubprocessErrstr(segcache_reset_subprocess)
                die(filename, "failed to reset SegCache; error: {}".format(segcache_reset_errstr))
        else:
            dump(filename, "the latest commit ID of SegCache is already {}".format(segcache_targetcommit))
    
    # segcache_install_dirpath = "{}/build".format(segcache_clone_dirpath)
    # if not os.path.exists(segcache_install_dirpath):
    #     prompt(filename, "install SegCache from source...")
    #     segcache_install_cmd = "cd {} && mkdir build && cd build && cmake .. && make -j".format(segcache_clone_dirpath)

    #     segcache_install_subprocess = runCmd(segcache_install_cmd)
    #     if segcache_install_subprocess.returncode != 0:
    #         segcache_install_errstr = getSubprocessErrstr(segcache_install_subprocess)
    #         die(filename, "failed to install {}; error: {}".format(segcache_install_dirpath, segcache_install_errstr))
    # else:
    #     dump(filename, "{} exists (SegCache has been installed)".format(segcache_install_dirpath))

    segcache_install_dirpath = "{}/src/cache/segcache/build".format(proj_dirpath)
    if not os.path.exists(segcache_install_dirpath):
        prompt(filename, "install SegCache from source...")
        segcache_install_cmd = "cd {} && mkdir build && cd build && cmake .. && make -j".format(segcache_clone_dirpath)

        segcache_install_subprocess = runCmd(segcache_install_cmd)
        if segcache_install_subprocess.returncode != 0:
            segcache_install_errstr = getSubprocessErrstr(segcache_install_subprocess)
            die(filename, "failed to install {}; error: {}".format(segcache_install_dirpath, segcache_install_errstr))
    else:
        dump(filename, "{} exists (SegCache has been installed)".format(segcache_install_dirpath))

# (8) Others: chown of libraries and update LD_LIBRARY_PATH

## Chown of libraries

prompt(filename, "chown of libraries...")
chown_cmd = "sudo chown -R {0}:{0} {1}".format(username, lib_dirpath)
chown_subprocess = runCmd(chown_cmd)
if chown_subprocess.returncode != 0:
    chown_errstr = getSubprocessErrstr(chown_subprocess)
    die(filename, "failed to chown of libraries; error: {}".format(chown_errstr))

## Update LD_LIBRARY_PATH

target_ld_libs = ["segcache", "cachelib", "x86_64-linux-gnu"]
target_ld_lib_dirpaths = ["{}/src/cache/segcache/build/ccommon/lib".format(proj_dirname), "{}/CacheLib/opt/cachelib/lib".format(lib_dirpath), "/usr/lib/x86_64-linux-gnu"]

prompt(filename, "check if need to update LD_LIBRARY_PATH...")
need_update_ld_library_path = False
# NOTE: LD_LIBRARY_PATH will NOT be passed into python interpreter, so we cannot get current_ld_library_path by os.environ.get("LD_LIBRARY_PATH") or runCmd("echo $LD_LIBRARY_PATH")
current_ld_library_path = sys.argv[1]
for tmp_lib in target_ld_libs:
    if tmp_lib not in current_ld_library_path:
        warn(filename, "LD_LIBRARY_PATH {} does not contain {}".format(current_ld_library_path, tmp_lib))
        need_update_ld_library_path = True
        break

if need_update_ld_library_path:
    prompt(filename, "find bash source filepath...")
    bashrc_source_filepath = os.path.join("/home/{}".format(username), ".bashrc")
    bashprofile_source_filepath = os.path.join("/home/{}".format(username), ".bash_profile")
    bash_source_filepath = ""
    if os.path.exists(bashrc_source_filepath):
        bash_source_filepath = bashrc_source_filepath
    elif os.path.exists(bashprofile_source_filepath):
        bash_source_filepath = bashprofile_source_filepath
    else:
        die(filename, "failed to find .bashrc or .bash_profile")

    prompt(filename, "check if need to update bash source file {}...".format(bash_source_filepath))
    need_update_bash_source_file = False
    # Check whether target libraries are contained in LD_LIBRARY_PATH at bash source file
    check_bash_source_file_cmd = "cat {} | grep \"LD_LIBRARY_PATH\"".format(bash_source_filepath)
    for i in range(len(target_ld_libs)):
        check_bash_source_file_cmd = "{} | grep \"{}\"".format(check_bash_source_file_cmd, target_ld_libs[i])
    check_bash_source_file_subprocess = runCmd(check_bash_source_file_cmd)
    if check_bash_source_file_subprocess.returncode == 0:
        check_bash_source_file_outputstr = getSubprocessOutputstr(check_bash_source_file_subprocess)
        if check_bash_source_file_outputstr == "":
            need_update_bash_source_file = True
        else:
            dump(filename, "bash source file {} already contains all target libraries".format(bash_source_filepath))
    else: # NOTE: if returncode is not 0, it also means that the target libraries are not contained in LD_LIBRARY_PATH at bash source file
        need_update_bash_source_file = True
        #die(filename, "failed to check bash source file {}".format(bash_source_filepath))
    
    if need_update_bash_source_file:
        prompt(filename, "update bash source file {}...".format(bash_source_filepath))
        update_bash_source_grepstr = ""
        for i in range(len(target_ld_lib_dirpaths)):
            if i == 0:
                update_bash_source_grepstr = "{}".format(target_ld_lib_dirpaths[i])
            elif i == len(target_ld_lib_dirpaths) - 1:
                update_bash_source_grepstr = "{}:{}:\$LD_LIBRARY_PATH".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])
            else:
                update_bash_source_grepstr = "{}:{}".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])
        update_bash_source_file_cmd = "echo \"export LD_LIBRARY_PATH={}\" >> {}".format(update_bash_source_grepstr, bash_source_filepath)
        update_bash_source_file_subprocess = runCmd(update_bash_source_file_cmd)
        if update_bash_source_file_subprocess.returncode != 0:
            update_bash_source_file_errstr = getSubprocessErrstr(update_bash_source_file_subprocess)
            die(filename, "failed to update bash source file {}; error: {}".format(bash_source_filepath, update_bash_source_file_errstr))

    # NOTE: as python will fork a non-interative bash to execute update_ld_library_path_cmd, it cannot find the built-in source command and also cannot change the environment variable of the interative bash launching the python script
    #prompt(filename, "source {} to update LD_LIBRARY_PATH...".format(bash_source_filepath))
    #update_ld_library_path_cmd = "source {}".format(bash_source_filepath)
    #update_ld_library_path_subprocess = runCmd(update_ld_library_path_cmd)
    #if update_ld_library_path_subprocess.returncode != 0:
    #    update_ld_library_path_outputstr = getSubprocessErrstr(update_ld_library_path_subprocess)
    #    die(filename, "failed to source {} to update LD_LIBRARY_PATH; error: {}".format(bash_source_filepath, update_ld_library_path_outputstr))

    emphasize(filename, "Please update LD_LIBRARY_PATH by this command: source {}".format(bash_source_filepath))
else:
    prompt(filename, "LD_LIBRARY_PATH alreay contains all target libraries")