#!/usr/bin/env python3
# install_lib: install third-party libraries automatically

import os
import sys

from ..common import *

is_clear_tarball = False # whether to clear intermediate tarball files

# Variables to control whether to install the corresponding softwares
is_install_xgboost = True # Used by GL-Cache
is_install_boost = True
is_install_cachelib = True
is_install_lrucache = True # Completely use hacked version
is_install_lfucache = True # Also including FIFO
is_install_rocksdb = True
is_install_smhasher = True
is_install_segcache = True # Completely use hacked version
is_install_webcachesim = True # Completely use hacked version
is_install_tommyds = True
is_install_lhd = True
is_install_s3fifo = True # Completely use hacked version (also including SIEVE)
is_install_glcache = True # Completely used hacked version
is_install_lrb = True
is_install_frozenhot = True
is_install_tragen = True
is_install_lacache = True
# is_install_adaptsize = True

# Check input CLI parameters
if len(sys.argv) != 2:
    LogUtil.die(Common.scriptname, "Usage: sudo python3 -m scripts.tools.install_lib :$LD_LIBRARY_PATH")

# (0) Install XGBoost (commit ID: c03a4d5) for GLCache
if is_install_xgboost:
    xgboost_clone_dirpath = "{}/xgboost".format(Common.lib_dirpath)
    xgboost_software_name = "XGBoost"
    xgboost_repo_url = "https://github.com/dmlc/xgboost.git"
    SubprocessUtil.cloneRepo(Common.scriptname, xgboost_clone_dirpath, xgboost_software_name, xgboost_repo_url, with_subrepo = True)

    xgboost_target_commit = "c03a4d5"
    SubprocessUtil.checkoutCommit(Common.scriptname, xgboost_clone_dirpath, xgboost_software_name, xgboost_target_commit)

    # Submodules (dmlc: ea21135; gputreeshap: 787259b)
    dmlc_target_commit = "ea21135"
    SubprocessUtil.checkoutCommit(Common.scriptname, os.path.join(xgboost_clone_dirpath, "dmlc-core"), "dmlc", dmlc_target_commit)
    gputreeshap_target_commit = "787259b"
    SubprocessUtil.checkoutCommit(Common.scriptname, os.path.join(xgboost_clone_dirpath, "gputreeshap"), "gputreeshap", gputreeshap_target_commit)

    xgboost_install_dirpath = "/usr/local/lib/libxgboost.so" # NOTE: lib/xgboost/build/cmake_install.cmake sets CMAKE_INSTALL_PREFIX as /usr/local by default to install header files, libxgboost.so, and XGBoostTargets.cmake
    xgboost_install_tool = "mkdir build && cd build && cmake .. && make -j && sudo make install"
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, xgboost_install_dirpath, xgboost_software_name, xgboost_clone_dirpath, xgboost_install_tool, time_consuming = True)
    print("")

# (1) Install boost 1.81.0

if is_install_boost:
    boost_download_filepath = "{}/boost-1.81.0.tar.gz".format(Common.lib_dirpath)
    boost_download_url = "https://github.com/boostorg/boost/releases/download/boost-1.81.0/boost-1.81.0.tar.gz"
    SubprocessUtil.downloadTarball(Common.scriptname, boost_download_filepath, boost_download_url)

    boost_decompress_dirpath = "{}/boost-1.81.0".format(Common.lib_dirpath)
    boost_decompress_tool = "tar -xzvf"
    SubprocessUtil.decompressTarball(Common.scriptname, boost_download_filepath, boost_decompress_dirpath, boost_decompress_tool)

    boost_install_dirpath = "{}/install".format(boost_decompress_dirpath)
    # (OBSOLETE) Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
    #boost_install_tool = "./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=/usr/local && sudo ./b2 install && sudo ldconfig"
    boost_install_tool = "./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=./install && sudo ./b2 install"
    SubprocessUtil.installDecompressedTarball(Common.scriptname, boost_decompress_dirpath, boost_install_dirpath, boost_install_tool, time_consuming = True)
    print("")

# (2) Install cachelib (commit ID: 7886d6d)

if is_install_cachelib:
    cachelib_clone_dirpath = "{}/CacheLib".format(Common.lib_dirpath)
    cachelib_software_name = "cachelib"
    cachelib_repo_url = "https://github.com/facebook/CacheLib.git"
    SubprocessUtil.cloneRepo(Common.scriptname, cachelib_clone_dirpath, cachelib_software_name, cachelib_repo_url)

    cachelib_target_commit = "7886d6d"
    SubprocessUtil.checkoutCommit(Common.scriptname, cachelib_clone_dirpath, cachelib_software_name, cachelib_target_commit)

    # Note: cachelib will first build third-party libs and then itself libs
    cachelib_cachebench_filepath = "{}/build-cachelib/cachebench/libcachelib_cachebench.a".format(cachelib_clone_dirpath)
    cachelib_allocator_filepath = "{}/opt/cachelib/lib/libcachelib_allocator.a".format(cachelib_clone_dirpath)
    if not os.path.exists(cachelib_cachebench_filepath) or not os.path.exists(cachelib_allocator_filepath):
        # Replace library dirpath in scripts/cachelib/build-package.sh before copying
        custom_cachelib_buildpkg_filepath = "scripts/cachelib/build-package.sh"
        default_lib_dirpath = "/home/sysheng/projects/covered-private/lib"
        PathUtil.replace_dir(Common.scriptname, default_lib_dirpath, Common.lib_dirpath, custom_cachelib_buildpkg_filepath)

        # Update build-package.sh for folly to use libboost 1.81.0
        LogUtil.prompt(Common.scriptname, "replace contrib/build-package.sh to use libboost 1.81.0...")
        replace_build_package_cmd = "sudo cp {} {}/contrib/build-package.sh".format(custom_cachelib_buildpkg_filepath, cachelib_clone_dirpath)
        replace_build_package_subprocess = SubprocessUtil.runCmd(replace_build_package_cmd)
        if replace_build_package_subprocess.returncode != 0:
            replace_build_package_errstr = SubprocessUtil.getSubprocessErrstr(replace_build_package_subprocess)
            LogUtil.die(Common.scriptname, "failed to replace contrib/build-package.sh (errmsg: {})".format(replace_build_package_errstr))
        
        # Restore library dirpath in scripts/cachelib/build-package.sh after copying
        PathUtil.restore_dir(Common.scriptname, default_lib_dirpath, Common.lib_dirpath, custom_cachelib_buildpkg_filepath)

        # Assert that liburing (liburing2 and liburing-dev) if exist MUST >= 2.3 for Ubuntu >= 23.04, or NOT exist for Ubuntu <= 20.04 (Facebook's libfolly does NOT support Ubuntu 22.04 with liburing 2.1)
        ## Check if we need liburing
        cachelib_pre_install_tool = None
        need_liburing = False
        check_need_liburing_cmd = "dpkg -l | grep liburing-dev"
        check_need_liburing_subprocess = SubprocessUtil.runCmd(check_need_liburing_cmd)
        if check_need_liburing_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_need_liburing_subprocess) != "":
            need_liburing = True
        if need_liburing: ## Verify liburing MUST >= 2.3 if need liburing
            need_upgrade_liburing, liburing_old_version = SubprocessUtil.checkVersion(Common.scriptname, "liburing", "2.3", "dpkg -s liburing-dev | grep ^Version: | sed -n 's/^Version: \([0-9\.]\+\)-.*/\\1/p'")
            if need_upgrade_liburing:
                LogUtil.die(Common.scriptname, "liburing {0} is NOT supported by Facebook's libfolly used in CacheLib -> please switch to compatible Ubuntu version (>= 23.04 with liburing >= 2.3 or <= 20.04 without liburing), or purge liburing {0} by dpkg".format(liburing_old_version))

        # Build cachelib and its dependencies
        #cachelib_install_tool = "./contrib/build.sh -j -T -v -S" # For debugging (NOTE: add -S for ./contrib/build.sh to skip git-clone/git-pull step if you have already downloaded external libs required by cachelib in lib/CacheLib/cachelib/external)
        cachelib_install_tool = "./contrib/build.sh -j -T -v"
        #cachelib_pre_install_tool = "sudo apt-get -y install liburing-dev" # Required by libfolly
        SubprocessUtil.installFromRepo(Common.scriptname, cachelib_software_name, cachelib_clone_dirpath, cachelib_install_tool, pre_install_tool = cachelib_pre_install_tool, time_consuming = True)
    else:
        LogUtil.dump(Common.scriptname, "cachelib has already been installed")
    print("")

# (3) Install LRU cache (commit ID: de1c4a0)

# NOTE: we just use LRU downloaded from github in lib/cpp-lru-cache as a reference, while always use our hacked version in src/cache/lru to support required interfaces
if is_install_lrucache:
    lrucache_clone_dirpath = "{}/cpp-lru-cache".format(Common.lib_dirpath)
    lrucache_software_name = "LRU cache"
    lrucache_repo_url = "https://github.com/lamerman/cpp-lru-cache.git"
    SubprocessUtil.cloneRepo(Common.scriptname, lrucache_clone_dirpath, lrucache_software_name, lrucache_repo_url)

    lrucache_target_commit = "de1c4a0"
    SubprocessUtil.checkoutCommit(Common.scriptname, lrucache_clone_dirpath, lrucache_software_name, lrucache_target_commit)
    print("")

# (4) Install LFU cache with FIFO (commit ID: 0f65db1)

if is_install_lfucache:
    lfucache_clone_dirpath = "{}/caches".format(Common.lib_dirpath)
    lfucache_software_name = "LFU cache"
    lfucache_repo_url = "https://github.com/vpetrigo/caches.git"
    SubprocessUtil.cloneRepo(Common.scriptname, lfucache_clone_dirpath, lfucache_software_name, lfucache_repo_url)

    lfucache_target_commit = "0f65db1"
    SubprocessUtil.checkoutCommit(Common.scriptname, lfucache_clone_dirpath, lfucache_software_name, lfucache_target_commit)
    print("")

# (5) Install RocksDB 8.1.1

if is_install_rocksdb:
    rocksdb_download_filepath="{}/rocksdb-8.1.1.tar.gz".format(Common.lib_dirpath)
    # NOTE: github url needs content-disposition to be converted into the the real url
    rocksdb_download_url = "--content-disposition https://github.com/facebook/rocksdb/archive/refs/tags/v8.1.1.tar.gz"
    SubprocessUtil.downloadTarball(Common.scriptname, rocksdb_download_filepath, rocksdb_download_url)

    rocksdb_decompress_dirpath = "{}/rocksdb-8.1.1".format(Common.lib_dirpath)
    rocksdb_decompress_tool = "tar -xzvf"
    SubprocessUtil.decompressTarball(Common.scriptname, rocksdb_download_filepath, rocksdb_decompress_dirpath, rocksdb_decompress_tool)

    rocksdb_install_dirpath = "{}/librocksdb.a".format(rocksdb_decompress_dirpath)
    rocksdb_install_tool = "PORTABLE=1 make static_lib"
    rocksdb_pre_install_tool = "sudo apt-get -y install libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libjemalloc-dev libsnappy-dev"
    SubprocessUtil.installDecompressedTarball(Common.scriptname, rocksdb_decompress_dirpath, rocksdb_install_dirpath, rocksdb_install_tool, pre_install_tool = rocksdb_pre_install_tool, time_consuming = True)

    if is_clear_tarball:
        SubprocessUtil.clearTarball(Common.scriptname, rocksdb_download_filepath)
    print("")

# (6) Install SMHasher (commit ID: 61a0530)

if is_install_smhasher:
    smhasher_clone_dirpath = "{}/smhasher".format(Common.lib_dirpath)
    smhasher_software_name = "SMHasher"
    smhasher_repo_url = "https://github.com/aappleby/smhasher.git"
    SubprocessUtil.cloneRepo(Common.scriptname, smhasher_clone_dirpath, smhasher_software_name, smhasher_repo_url)

    smhasher_target_commit = "61a0530"
    SubprocessUtil.checkoutCommit(Common.scriptname, smhasher_clone_dirpath, smhasher_software_name, smhasher_target_commit)
    print("")

# (7) Install SegCache (commit ID: 0abdfee)

# NOTE: we just use SegCache downloaded from github in lib/segcache as a reference, while always use our hacked version in src/cache/segcache to fix SegCache's implementation issues
if is_install_segcache:
    segcache_clone_dirpath = "{}/Segcache".format(Common.lib_dirpath)
    segcache_software_name = "SegCache"
    segcache_repo_url = "https://github.com/Thesys-lab/Segcache.git"
    SubprocessUtil.cloneRepo(Common.scriptname, segcache_clone_dirpath, segcache_software_name, segcache_repo_url)

    segcache_target_commit = "0abdfee"
    SubprocessUtil.checkoutCommit(Common.scriptname, segcache_clone_dirpath, segcache_software_name, segcache_target_commit)
    
    # NOTE: use the hacked version to install segcache
    #segcache_install_dirpath = "{}/build".format(segcache_clone_dirpath)
    segcache_install_dirpath = "{}/src/cache/segcache/build".format(Common.proj_dirname)
    tmp_segcache_clone_dirpath = os.path.dirname(segcache_install_dirpath)
    segcache_install_tool = "mkdir build && cd build && cmake .. && make -j"
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, segcache_install_dirpath, segcache_software_name, tmp_segcache_clone_dirpath, segcache_install_tool)
    print("")

# (7) Install WebCacheSim (including GDSF and AdaptSize) (commit ID: 8818442)

# NOTE: we just use GDSF and AdaptSize downloaded from github in lib/webcachesim as a reference, while always use our hacked version in src/cache/greedydual and src/cache/adaptsize to support required interfaces
if is_install_webcachesim:
    webcachesim_clone_dirpath = "{}/webcachesim".format(Common.lib_dirpath)
    webcachesim_software_name = "WebCacheSim"
    webcachesim_repo_url = "https://github.com/dasebe/webcachesim.git"
    SubprocessUtil.cloneRepo(Common.scriptname, webcachesim_clone_dirpath, webcachesim_software_name, webcachesim_repo_url)
    
    webcachesim_target_commit = "8818442"
    SubprocessUtil.checkoutCommit(Common.scriptname, webcachesim_clone_dirpath, webcachesim_software_name, webcachesim_target_commit)
    print("")

# (8) Install TommyDS (commit ID: 97ff743)

if is_install_tommyds:
    tommyds_clone_dirpath = "{}/tommyds".format(Common.lib_dirpath)
    tommyds_software_name = "TommyDS"
    tommyds_repo_url = "https://github.com/amadvance/tommyds.git"
    SubprocessUtil.cloneRepo(Common.scriptname, tommyds_clone_dirpath, tommyds_software_name, tommyds_repo_url)

    tommyds_target_commit = "97ff743"
    SubprocessUtil.checkoutCommit(Common.scriptname, tommyds_clone_dirpath, tommyds_software_name, tommyds_target_commit)

    # NOTE: use Makefile of TommyDS itself due to using gcc instead of g++, where gcc does NOT add prefix/suffix to variable names (e.g., functions and classes)
    tommyds_install_dirpath = "{}/libtommy.a".format(tommyds_clone_dirpath)
    tommyds_install_tool = "make all && mv {0}/tommy.o {0}/libtommy.a".format(tommyds_clone_dirpath)
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, tommyds_install_dirpath, tommyds_software_name, tommyds_clone_dirpath, tommyds_install_tool)
    print("")

# (9) Install LHD (commit ID: 806ef46)

if is_install_lhd:
    lhd_clone_dirpath = "{}/lhd".format(Common.lib_dirpath)
    lhd_software_name = "LHD"
    lhd_repo_url = "https://github.com/CMU-CORGI/LHD.git"
    SubprocessUtil.cloneRepo(Common.scriptname, lhd_clone_dirpath, lhd_software_name, lhd_repo_url)

    lhd_target_commit = "806ef46"
    SubprocessUtil.checkoutCommit(Common.scriptname, lhd_clone_dirpath, lhd_software_name, lhd_target_commit)
    print("")

# (10) Install S3FIFO with SIEVE, ARC, TinyLFU, and SLRU (commit ID: 79fde46)

# NOTE: we just use S3FIFO and SIEVE downloaded from github in lib/s3fifo as references, while always use our hacked version in src/cache/s3fifo and src/cache/sieve to escape libcachesim for edge settings and support required interfaces
if is_install_s3fifo:
    s3fifo_clone_dirpath = "{}/s3fifo".format(Common.lib_dirpath)
    s3fifo_software_name = "S3FIFO"
    s3fifo_repo_url = "https://github.com/Thesys-lab/sosp23-s3fifo.git"
    SubprocessUtil.cloneRepo(Common.scriptname, s3fifo_clone_dirpath, s3fifo_software_name, s3fifo_repo_url)

    s3fifo_target_commit = "79fde46"
    SubprocessUtil.checkoutCommit(Common.scriptname, s3fifo_clone_dirpath, s3fifo_software_name, s3fifo_target_commit)
    print("")

# (11) Install GLCache (commit ID: fbb8240)

# NOTE: we just use GLCache downloaded from github in lib/glcache as a reference, while always use our hacked version in src/cache/glcache to fix GLCache's implementation issues
if is_install_glcache:
    glcache_clone_dirpath = "{}/glcache".format(Common.lib_dirpath)
    glcache_software_name = "GLCache"
    glcache_repo_url = "https://github.com/Thesys-lab/fast23-GLCache.git"
    SubprocessUtil.cloneRepo(Common.scriptname, glcache_clone_dirpath, glcache_software_name, glcache_repo_url)

    glcache_target_commit = "fbb8240"
    SubprocessUtil.checkoutCommit(Common.scriptname, glcache_clone_dirpath, glcache_software_name, glcache_target_commit)

    # NOTE: use the hacked version to install glcache
    #glcache_microimpl_clone_dirpath = "{}/micro-implementation".format(glcache_clone_dirpath)
    glcache_microimpl_clone_dirpath = "{}/src/cache/glcache/micro-implementation".format(Common.proj_dirname)
    glcache_install_dirpath = "{}/build/lib/liblibCacheSim.so".format(glcache_microimpl_clone_dirpath)
    glcache_install_tool = "mkdir _build && cd _build && cmake -DSUPPORT_ZSTD_TRACE=OFF -DCMAKE_INSTALL_PREFIX=../build .. && make -j && sudo make install" # NOTE: we do NOT use zstd-compressed traces and will install glcache into src/cache/glcache/micro-implementation/build/
    glcache_preinstall_tool = "sudo apt-get -y install libglib2.0-dev libgoogle-perftools-dev"
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, glcache_install_dirpath, glcache_software_name, glcache_microimpl_clone_dirpath, glcache_install_tool, pre_install_tool = glcache_preinstall_tool)
    print("")

# (12) Install LRB (commit ID: 9e8b442)

if is_install_lrb:
    lrb_clone_dirpath = "{}/lrb".format(Common.lib_dirpath)
    lrb_software_name = "LRB"
    lrb_repo_url = "https://github.com/sunnyszy/lrb.git"
    SubprocessUtil.cloneRepo(Common.scriptname, lrb_clone_dirpath, lrb_software_name, lrb_repo_url, with_subrepo = True)

    lrb_target_commit = "9e8b442"
    SubprocessUtil.checkoutCommit(Common.scriptname, lrb_clone_dirpath, lrb_software_name, lrb_target_commit)

    # Overwrite lib/lrb/src/CMakeLists.txt to fix errors of cmake files provided by authors
    original_lrb_cmakelists_filepath = "{}/lrb/src/CMakeLists.txt".format(Common.lib_dirpath)
    correct_lrb_cmakelists_filepath = "scripts/lrb/CMakeLists.txt"
    LogUtil.prompt(Common.scriptname, "Overwrite {} by {}...".format(original_lrb_cmakelists_filepath, correct_lrb_cmakelists_filepath))
    overwrite_lrb_cmakelists_cmd = "cp {} {}".format(correct_lrb_cmakelists_filepath, original_lrb_cmakelists_filepath)
    overwrite_lrb_cmakelists_subprocess = SubprocessUtil.runCmd(overwrite_lrb_cmakelists_cmd)
    if overwrite_lrb_cmakelists_subprocess.returncode != 0:
        overwrite_lrb_cmakelists_errstr = SubprocessUtil.getSubprocessErrstr(overwrite_lrb_cmakelists_subprocess)
        LogUtil.die(Common.scriptname, "failed to overwrite {} by {} (errmsg: {})".format(original_lrb_cmakelists_filepath, correct_lrb_cmakelists_filepath, overwrite_lrb_cmakelists_errstr))

    # Install LRB
    lrb_install_dirpath = "{}/install".format(lrb_clone_dirpath)
    lrb_install_tool = "bash {}/scripts/lrb/install.sh".format(Common.proj_dirname)
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, lrb_install_dirpath, lrb_software_name, lrb_clone_dirpath, lrb_install_tool, time_consuming = True)
    print("")

# (13) Install FrozenHot (commit ID: eabb2b9)

if is_install_frozenhot:
    frozenhot_clone_dirpath = "{}/frozenhot".format(Common.lib_dirpath)
    frozenhot_software_name = "FrozenHot"
    frozenhot_repo_url = "https://github.com/ziyueqiu/FrozenHot.git"
    SubprocessUtil.cloneRepo(Common.scriptname, frozenhot_clone_dirpath, frozenhot_software_name, frozenhot_repo_url, with_subrepo = True)

    frozenhot_target_commit = "eabb2b9"
    SubprocessUtil.checkoutCommit(Common.scriptname, frozenhot_clone_dirpath, frozenhot_software_name, frozenhot_target_commit)

    # Submodules (CLHT: 2240b69)
    clht_target_commit = "2240b69"
    SubprocessUtil.checkoutCommit(Common.scriptname, os.path.join(frozenhot_clone_dirpath, "CLHT"), "CLHT", clht_target_commit)

    # Compile CLHT
    clht_target_filepath = "{}/CLHT/libclht.a".format(frozenhot_clone_dirpath)
    if not os.path.exists(clht_target_filepath):
        LogUtil.prompt(Common.scriptname, "compile CLHT...")
        clht_compile_cmd = "cd {}/CLHT && ./prepare_everything.sh".format(frozenhot_clone_dirpath)
        clht_compile_subprocess = SubprocessUtil.runCmd(clht_compile_cmd)
        if clht_compile_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, "failed to compile CLHT (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(clht_compile_subprocess)))

    # Install FrozenHot (still compile the executable binary in FrozenHot to install third-party libs, although the executable binary is not used in COVERED project)
    frozenhot_install_dirpath = "{}/build/test_trace".format(frozenhot_clone_dirpath)
    frozenhot_install_tool = "./prepare.sh"
    frozenhot_preinstall_tool = "sudo apt-get -y install libtbb-dev numactl"
    SubprocessUtil.installFromRepoIfNot(Common.scriptname, frozenhot_install_dirpath, frozenhot_software_name, frozenhot_clone_dirpath, frozenhot_install_tool, pre_install_tool = frozenhot_preinstall_tool)
    print("")

# (14) Install TRAGEN (commit ID: 70cd5bb)

if is_install_tragen:
    tragen_clone_dirpath = "{}/tragen".format(Common.lib_dirpath)
    tragen_software_name = "TRAGEN"
    tragen_repo_url = "https://github.com/UMass-LIDS/Tragen.git"
    SubprocessUtil.cloneRepo(Common.scriptname, tragen_clone_dirpath, tragen_software_name, tragen_repo_url)

    tragen_target_commit = "70cd5bb"
    SubprocessUtil.checkoutCommit(Common.scriptname, tragen_clone_dirpath, tragen_software_name, tragen_target_commit)

    # NOTE: TRAGEN is based on python, which does not need compilation and installation

    # Overwrite hacked part into tragen repo to generate geo-distributed caching workloads
    tragen_overwrite_cmd = "cp {}/scripts/tragen/*.py {}/".format(Common.proj_dirname, tragen_clone_dirpath)
    tragen_overwrite_subprocess = SubprocessUtil.runCmd(tragen_overwrite_cmd)
    if tragen_overwrite_subprocess.returncode != 0:
        tragen_overwrite_errstr = SubprocessUtil.getSubprocessErrstr(tragen_overwrite_subprocess)
        LogUtil.die(Common.scriptname, "failed to overwrite hacked part into tragen repo (errmsg: {})".format(tragen_overwrite_errstr))

# (15) Install LACache (commit ID: 2dcb806)

if is_install_lacache:
    lacache_clone_dirpath = "{}/lacache".format(Common.lib_dirpath)
    lacache_software_name = "LA-Cache"
    lacache_repo_url = "https://github.com/GYan58/la-cache.git"
    SubprocessUtil.cloneRepo(Common.scriptname, lacache_clone_dirpath, lacache_software_name, lacache_repo_url)

    lacache_target_commit = "2dcb806"
    SubprocessUtil.checkoutCommit(Common.scriptname, lacache_clone_dirpath, lacache_software_name, lacache_target_commit)

    # NOTE: NO need to install LA-Cache source code, as we refer to the source code to implement a practical version in src/cache/lacache for cooperative caching

# (16) Others: chown of libraries and update LD_LIBRARY_PATH

## (16.1) Chown of libraries

LogUtil.prompt(Common.scriptname, "chown of libraries...")
chown_cmd = "sudo chown -R {0}:{0} {1}".format(Common.username, Common.lib_dirpath)
chown_subprocess = SubprocessUtil.runCmd(chown_cmd)
if chown_subprocess.returncode != 0:
    chown_errstr = SubprocessUtil.getSubprocessErrstr(chown_subprocess)
    LogUtil.die(Common.scriptname, "failed to chown of libraries (errmsg: {})".format(chown_errstr))

## (16.2) Update LD_LIBRARY_PATH for interactive and non-interactive shells

target_ld_libs = ["webcachesim", "libbf", "mongocxxdriver", "lightgbm", "glcache", "segcache", "cachelib", "boost", "x86_64-linux-gnu"]
target_ld_lib_dirpaths = ["{}/lrb/install/webcachesim/lib".format(Common.lib_dirpath), "{}/lrb/install/libbf/lib".format(Common.lib_dirpath), "{}/lrb/install/mongocxxdriver/lib".format(Common.lib_dirpath), "{}/lrb/install/lightgbm/lib".format(Common.lib_dirpath), "{}/src/cache/glcache/micro-implementation/build/lib".format(Common.proj_dirname), "{}/src/cache/segcache/build/ccommon/lib".format(Common.proj_dirname), "{}/CacheLib/opt/cachelib/lib".format(Common.lib_dirpath), "{}/boost-1.81.0/install/lib".format(Common.lib_dirpath), "/usr/lib/x86_64-linux-gnu"]

# Formulate grepstr to check LD_LIBRARY_PATH in {~/.bashrc or ~/.bash_profile} and ~/.bashrc_non_interactive
check_bash_source_grepstr = "grep \"LD_LIBRARY_PATH\""
for i in range(len(target_ld_libs)):
    check_bash_source_grepstr = "{} | grep \"{}\"".format(check_bash_source_grepstr, target_ld_libs[i])

# Formulate grepstr for LD_LIBRARY_PATH to be appended into {~/.bashrc or ~/.bash_profile} and ~/.bashrc_non_interactive if necessary
update_bash_source_grepstr = ""
for i in range(len(target_ld_lib_dirpaths)):
    if i == 0:
        update_bash_source_grepstr = "{}".format(target_ld_lib_dirpaths[i])
    elif i == len(target_ld_lib_dirpaths) - 1:
        # NOTE: MUST use double braces to avoid python string format processing
        update_bash_source_grepstr = "{}:{}:${{LD_LIBRARY_PATH}}".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])
    else:
        update_bash_source_grepstr = "{}:{}".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])

### (16.3) Update LD_LIBRARY_PATH for non-interactive shells

noninteractive_bash_source_filepath = "/home/{}/.bashrc_non_interactive".format(Common.username)

# Check whether target libraries are contained in LD_LIBRARY_PATH at non-interactive bash source file
LogUtil.prompt(Common.scriptname, "check if need to update LD_LIBRARY_PATH for non-interactive shells...")
need_update_noninteractive_bash_source_file = False;
check_noninteractive_bash_source_file_cmd = "cat {} | {}".format(noninteractive_bash_source_filepath, check_bash_source_grepstr)
check_noninteractive_bash_source_file_subprocess = SubprocessUtil.runCmd(check_noninteractive_bash_source_file_cmd)
if check_noninteractive_bash_source_file_subprocess.returncode == 0:
    check_noninteractive_bash_source_file_outputstr = SubprocessUtil.getSubprocessOutputstr(check_noninteractive_bash_source_file_subprocess)
    if check_noninteractive_bash_source_file_outputstr == "":
        need_update_noninteractive_bash_source_file = True
    else:
        LogUtil.dump(Common.scriptname, "non-interactive bash source file {} already contains all target libraries".format(noninteractive_bash_source_filepath))
else: # NOTE: if returncode is not 0, it also means that the target libraries are not contained in LD_LIBRARY_PATH at non-interactive bash source file
    need_update_noninteractive_bash_source_file = True

if need_update_noninteractive_bash_source_file:
    LogUtil.prompt(Common.scriptname, "create non-interactive bash source file {}...".format(noninteractive_bash_source_filepath))

    # Remove old non-interactive bash source file if any
    remove_old_noninteractive_bash_source_file_cmd = "rm {}".format(noninteractive_bash_source_filepath)
    remove_old_noninteractive_bash_source_file_subprocess = SubprocessUtil.runCmd(remove_old_noninteractive_bash_source_file_cmd)

    # Create new non-interactive bash source file with correct LD_LIBRARY_PATH
    # NOTE: use single quotes (or \$) instead of double quotes here, otherwise ${LD_LIBRARY_PATH} in update_bash_source_grepstr will not be processed literally
    create_noninteractive_bash_source_file_cmd = "echo 'export LD_LIBRARY_PATH={}' >> {}".format(update_bash_source_grepstr, noninteractive_bash_source_filepath)
    create_noninteractive_bash_source_file_subprocess = SubprocessUtil.runCmd(create_noninteractive_bash_source_file_cmd)
    if create_noninteractive_bash_source_file_subprocess.returncode != 0:
        create_noninteractive_bash_source_file_errstr = SubprocessUtil.getSubprocessErrstr(create_noninteractive_bash_source_file_subprocess)
        LogUtil.die(Common.scriptname, "failed to create non-interactive bash source file {} (errmsg: {})".format(noninteractive_bash_source_filepath, create_noninteractive_bash_source_file_errstr))

### (15.4) Update LD_LIBRARY_PATH for interactive shells

LogUtil.prompt(Common.scriptname, "check if need to update LD_LIBRARY_PATH for interactive shells...")
need_update_ld_library_path = False
# NOTE: LD_LIBRARY_PATH will NOT be passed into python interpreter, so we cannot get current_ld_library_path by os.environ.get("LD_LIBRARY_PATH") or SubprocessUtil.runCmd("echo $LD_LIBRARY_PATH")
current_ld_library_path = sys.argv[1]
for tmp_lib in target_ld_libs:
    if tmp_lib not in current_ld_library_path:
        LogUtil.warn(Common.scriptname, "LD_LIBRARY_PATH {} does not contain {}".format(current_ld_library_path, tmp_lib))
        need_update_ld_library_path = True
        break

if need_update_ld_library_path:
    LogUtil.prompt(Common.scriptname, "find bash source filepath...")
    bashrc_source_filepath = os.path.join("/home/{}".format(Common.username), ".bashrc")
    bashprofile_source_filepath = os.path.join("/home/{}".format(Common.username), ".bash_profile")
    bash_source_filepath = ""
    if os.path.exists(bashrc_source_filepath):
        bash_source_filepath = bashrc_source_filepath
    elif os.path.exists(bashprofile_source_filepath):
        bash_source_filepath = bashprofile_source_filepath
    else:
        LogUtil.die(Common.scriptname, "failed to find .bashrc or .bash_profile")

    LogUtil.prompt(Common.scriptname, "check if need to update bash source file {}...".format(bash_source_filepath))
    need_update_bash_source_file = False
    # Check whether target libraries are contained in LD_LIBRARY_PATH at bash source file
    check_bash_source_file_cmd = "cat {} | {}".format(bash_source_filepath, check_bash_source_grepstr)
    check_bash_source_file_subprocess = SubprocessUtil.runCmd(check_bash_source_file_cmd)
    if check_bash_source_file_subprocess.returncode == 0:
        check_bash_source_file_outputstr = SubprocessUtil.getSubprocessOutputstr(check_bash_source_file_subprocess)
        if check_bash_source_file_outputstr == "":
            need_update_bash_source_file = True
        else:
            LogUtil.dump(Common.scriptname, "bash source file {} already contains all target libraries".format(bash_source_filepath))
    else: # NOTE: if returncode is not 0, it also means that the target libraries are not contained in LD_LIBRARY_PATH at bash source file
        need_update_bash_source_file = True
        #LogUtil.die(Common.scriptname, "failed to check bash source file {} (errmsg: {})".format(bash_source_filepath, SubprocessUtil.getSubprocessErrstr(check_bash_source_file_subprocess)))
    
    if need_update_bash_source_file:
        LogUtil.prompt(Common.scriptname, "update bash source file {}...".format(bash_source_filepath))
        # NOTE: use single quotes (or \$) instead of double quotes here, otherwise ${LD_LIBRARY_PATH} in update_bash_source_grepstr will not be processed literally
        update_bash_source_file_cmd = "echo 'export LD_LIBRARY_PATH={}' >> {}".format(update_bash_source_grepstr, bash_source_filepath)
        update_bash_source_file_subprocess = SubprocessUtil.runCmd(update_bash_source_file_cmd)
        if update_bash_source_file_subprocess.returncode != 0:
            update_bash_source_file_errstr = SubprocessUtil.getSubprocessErrstr(update_bash_source_file_subprocess)
            LogUtil.die(Common.scriptname, "failed to update bash source file {} (errmsg: {})".format(bash_source_filepath, update_bash_source_file_errstr))

    # NOTE: as python will fork a non-interative bash to execute update_ld_library_path_cmd, it cannot find the built-in source command and also cannot change the environment variable of the interative bash launching the python script
    #LogUtil.prompt(Common.scriptname, "source {} to update LD_LIBRARY_PATH...".format(bash_source_filepath))
    #update_ld_library_path_cmd = "source {}".format(bash_source_filepath)
    #update_ld_library_path_subprocess = SubprocessUtil.runCmd(update_ld_library_path_cmd)
    #if update_ld_library_path_subprocess.returncode != 0:
    #    update_ld_library_path_errstr = SubprocessUtil.getSubprocessErrstr(update_ld_library_path_subprocess)
    #    LogUtil.die(Common.scriptname, "failed to source {} to update LD_LIBRARY_PATH (errmsg: {})".format(bash_source_filepath, update_ld_library_path_errstr))

    LogUtil.emphasize(Common.scriptname, "Please update LD_LIBRARY_PATH by this command: source {}".format(bash_source_filepath))
else:
    LogUtil.dump(Common.scriptname, "LD_LIBRARY_PATH alreay contains all target libraries")





# NOTE: OBSOLETE due to strictly coupled with Varnish
# # (16) Install AdaptSize (commit ID: 6deab4b)

# if is_install_adaptsize:
#     adaptsize_clone_dirpath = "{}/adaptsize".format(Common.lib_dirpath)
#     adaptsize_software_name = "AdaptSize"
#     adaptsize_repo_url = "https://github.com/dasebe/AdaptSize.git"
#     SubprocessUtil.cloneRepo(Common.scriptname, adaptsize_clone_dirpath, adaptsize_software_name, adaptsize_repo_url)

#     adaptsize_target_commit = "6deab4b"
#     SubprocessUtil.checkoutCommit(Common.scriptname, adaptsize_clone_dirpath, adaptsize_software_name, adaptsize_target_commit)

#     # Execute "sudo apt-get install -y autotools-dev make automake libtool pkg-config" for dependencies of Varnish and hence AdaptSize
#     LogUtil.prompt(Common.scriptname, "install dependencies for AdaptSize...")
#     dependency_install_cmd = "sudo apt-get install -y autotools-dev make automake libtool pkg-config autoconf libjemalloc-dev libedit-dev libncurses-dev libpcre3-dev python-docutils python-sphinx graphviz"
#     dependency_install_subprocess = SubprocessUtil.runCmd(dependency_install_cmd, is_capture_output = False)
#     if dependency_install_subprocess.returncode != 0:
#         LogUtil.die(Common.scriptname, "failed to install dependencies for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(dependency_install_subprocess)))
    
#     # Install AdaptSize
#     varnish_tarball_filepath = "{}/varnish-4.1.2.tgz".format(adaptsize_clone_dirpath)
#     varnish_decompress_dirpath = "{}/varnish-4.1.2".format(adaptsize_clone_dirpath)
#     varnish_install_dirpath = "{}/varnish".format(varnish_decompress_dirpath)
#     if not os.path.exists(varnish_install_dirpath):
#         # Download varnish
#         if not os.path.exists(varnish_tarball_filepath):
#             LogUtil.prompt(Common.scriptname, "download varnish for AdaptSize...")
#             download_varnish_cmd = "cd {} && wget https://varnish-cache.org/downloads/varnish-4.1.2.tgz".format(adaptsize_clone_dirpath)
#             download_varnish_subprocess = SubprocessUtil.runCmd(download_varnish_cmd, is_capture_output = False)
#             if download_varnish_subprocess.returncode != 0:
#                 LogUtil.die(Common.scriptname, "failed to download varnish for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(download_varnish_subprocess)))
        
#         # Decompress and patch varnish
#         if not os.path.exists(varnish_decompress_dirpath):
#             # Decompress varnish
#             LogUtil.prompt(Common.scriptname, "decompress varnish for AdaptSize...")
#             decompress_varnish_cmd = "cd {} && tar xfvz varnish-4.1.2.tgz".format(adaptsize_clone_dirpath)
#             decompress_varnish_subprocess = SubprocessUtil.runCmd(decompress_varnish_cmd)
#             if decompress_varnish_subprocess.returncode != 0:
#                 LogUtil.die(Common.scriptname, "failed to decompress varnish for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(decompress_varnish_subprocess)))
            
#             # Patch varnish
#             LogUtil.prompt(Common.scriptname, "patch varnish for AdaptSize...")
#             patch_varnish_cmd = "cd {} && patch varnish-4.1.2/bin/varnishd/cache/cache_req_fsm.c < VarnishPatches/cache_req_fsm.patch && patch varnish-4.1.2/include/tbl/params.h < VarnishPatches/params.patch && patch varnish-4.1.2/lib/libvarnishapi/vsl_dispatch.c < VarnishPatches/vsl_dispatch.patch".format(adaptsize_clone_dirpath)
#             patch_varnish_subprocess = SubprocessUtil.runCmd(patch_varnish_cmd)
#             if patch_varnish_subprocess.returncode != 0:
#                 LogUtil.die(Common.scriptname, "failed to patch varnish for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(patch_varnish_subprocess)))
        
#         is_adaptsize_install_success = True
#         adaptsize_install_errmsg = ""
        
#         # Install varnish
#         if is_adaptsize_install_success:
#             LogUtil.prompt(Common.scriptname, "install varnish for AdaptSize...")
#             install_varnish_cmd = "cd {} && ./configure --prefix={} && make && make install".format(varnish_decompress_dirpath, varnish_install_dirpath)
#             install_varnish_subprocess = SubprocessUtil.runCmd(install_varnish_cmd, is_capture_output = False)
#             if install_varnish_subprocess.returncode != 0:
#                 is_adaptsize_install_success = False
#                 adaptsize_install_errmsg = "failed to install varnish for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(install_varnish_subprocess))
        
#         # Compile and install AdaptSize Vmod
#         if is_adaptsize_install_success:
#             LogUtil.prompt(Common.scriptname, "compile and install AdaptSize Vmod for AdaptSize...")
#             install_vmod_cmd = "cd {0}/AdaptSizeVmod && export PKG_CONFIG_PATH={1}/lib/pkgconfig && ./autogen.sh --prefix={1} && ./configure --prefix={1} && make && make install".format(adaptsize_clone_dirpath, varnish_install_dirpath)
#             install_vmod_subprocess = SubprocessUtil.runCmd(install_vmod_cmd, is_capture_output = False)
#             if install_vmod_subprocess.returncode != 0:
#                 is_adaptsize_install_success = False
#                 adaptsize_install_errmsg = "failed to compile and install AdaptSize Vmod for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(install_vmod_subprocess))
        
#         # Overwrite adaptsize.c and Makefile to fix compilation errors of tuning module
#         if is_adaptsize_install_success:
#             original_tuner_dirpath = "{}/AdaptSizeTuner".format(adaptsize_clone_dirpath)
#             correct_tuner_dirpath = "{}/scripts/adaptsize".format(Common.proj_dirname)
#             overwrite_filenames = ["adaptsize.c", "Makefile"]
#             for tmp_filename in overwrite_filenames:
#                 original_tuner_filepath = "{}/{}".format(original_tuner_dirpath, tmp_filename)
#                 correct_tuner_filepath = "{}/{}".format(correct_tuner_dirpath, tmp_filename)
#                 LogUtil.prompt(Common.scriptname, "overwrite {} to {} to fix compilation errors of tuning module...".format(original_tuner_filepath, correct_tuner_filepath))
#                 overwrite_tuner_cmd = "cp {} {}".format(correct_tuner_filepath, original_tuner_filepath)
#                 overwrite_tuner_subprocess = SubprocessUtil.runCmd(overwrite_tuner_cmd)
#                 if overwrite_tuner_subprocess.returncode != 0:
#                     is_adaptsize_install_success = False
#                     adaptsize_install_errmsg = "failed to overwrite {} to {} to fix compilation errors of tuning module (errmsg: {})".format(correct_tuner_filepath, original_tuner_filepath, SubprocessUtil.getSubprocessErrstr(overwrite_tuner_subprocess))
        
#         # Compile AdaptSize tuning module
#         if is_adaptsize_install_success:
#             LogUtil.prompt(Common.scriptname, "compile AdaptSize tuning module for AdaptSize...")
#             install_tuner_cmd = "cd {}/AdaptSizeTuner && make".format(adaptsize_clone_dirpath)
#             install_tuner_subprocess = SubprocessUtil.runCmd(install_tuner_cmd, is_capture_output = False)
#             if install_tuner_subprocess.returncode != 0:
#                 is_adaptsize_install_success = False
#                 adaptsize_install_errmsg = "failed to compile AdaptSize tuning module for AdaptSize (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(install_tuner_subprocess))
        
#         # Clear if not install AdaptSize successfully
#         if not is_adaptsize_install_success:
#             remove_varnish_install_dirpath = "rm -rf {}".format(varnish_install_dirpath)
#             remove_varnish_install_subprocess = SubprocessUtil.runCmd(remove_varnish_install_dirpath)
#             if remove_varnish_install_subprocess.returncode != 0:
#                 LogUtil.die(Common.scriptname, "failed to remove varnish install dirpath {} (errmsg: {})".format(varnish_install_dirpath, SubprocessUtil.getSubprocessErrstr(remove_varnish_install_subprocess)))
            
#             LogUtil.die(Common.scriptname, adaptsize_install_errmsg)