#!/usr/bin/env python3
# install_lib: install third-party libraries automatically

import os
import sys

from ..common import *

is_clear_tarball = False # whether to clear intermediate tarball files

# Variables to control whether to install the corresponding softwares
is_install_boost = True
is_install_cachelib = True
is_install_lrucache = True # Completely use hacked version
is_install_lfucache = True
is_install_rocksdb = True
is_install_smhasher = True
is_install_segcache = True # Completely use hacked version
is_install_gdsf = True # Completely use hacked version
is_install_tommyds = True
is_install_lhd = True

# (0) Check input CLI parameters

if len(sys.argv) != 2:
    die(scriptname, "Usage: sudo python3 -m scripts.tools.install_lib :$LD_LIBRARY_PATH")

# (1) Install boost 1.81.0

if is_install_boost:
    boost_download_filepath = "{}/boost_1_81_0.tar.gz".format(lib_dirpath)
    boost_download_url = "https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz"
    downloadTarball(scriptname, boost_download_filepath, boost_download_url)

    boost_decompress_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
    boost_decompress_tool = "tar -xzvf"
    decompressTarball(scriptname, boost_download_filepath, boost_decompress_dirpath, boost_decompress_tool)

    boost_install_dirpath = "{}/install".format(boost_decompress_dirpath)
    # (OBSOLETE) Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
    #boost_install_tool = "./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=/usr/local && sudo ./b2 install && sudo ldconfig"
    boost_install_tool = "./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json,stacktrace,context --prefix=./install && sudo ./b2 install"
    installDecompressedTarball(scriptname, boost_decompress_dirpath, boost_install_dirpath, boost_install_tool, time_consuming = True)
    print("")

# (2) Install cachelib (commit ID: 7886d6d)

if is_install_cachelib:
    cachelib_clone_dirpath = "{}/CacheLib".format(lib_dirpath)
    cachelib_software_name = "cachelib"
    cachelib_repo_url = "https://github.com/facebook/CacheLib.git"
    cloneRepo(scriptname, cachelib_clone_dirpath, cachelib_software_name, cachelib_repo_url)

    cachelib_target_commit = "7886d6d"
    checkoutCommit(scriptname, cachelib_clone_dirpath, cachelib_software_name, cachelib_target_commit)

    # Note: cachelib will first build third-party libs and then itself libs
    cachelib_cachebench_filepath = "{}/build-cachelib/cachebench/libcachelib_cachebench.a".format(cachelib_clone_dirpath)
    cachelib_allocator_filepath = "{}/opt/cachelib/lib/libcachelib_allocator.a".format(cachelib_clone_dirpath)
    if not os.path.exists(cachelib_cachebench_filepath) or not os.path.exists(cachelib_allocator_filepath):
        # Replace library dirpath in scripts/cachelib/build-package.sh before copying
        custom_cachelib_buildpkg_filepath = "scripts/cachelib/build-package.sh"
        default_lib_dirpath = "/home/sysheng/projects/covered-private/lib"
        replace_dir(scriptname, default_lib_dirpath, lib_dirpath, custom_cachelib_buildpkg_filepath)

        # Update build-package.sh for folly to use libboost 1.81.0
        prompt(scriptname, "replace contrib/build-package.sh to use libboost 1.81.0...")
        replace_build_package_cmd = "sudo cp {} {}/contrib/build-package.sh".format(custom_cachelib_buildpkg_filepath, cachelib_clone_dirpath)
        replace_build_package_subprocess = runCmd(replace_build_package_cmd)
        if replace_build_package_subprocess.returncode != 0:
            replace_build_package_errstr = getSubprocessErrstr(replace_build_package_subprocess)
            die(scriptname, "failed to replace contrib/build-package.sh; error: {}".format(replace_build_package_errstr))
        
        # Restore library dirpath in scripts/cachelib/build-package.sh after copying
        restore_dir(scriptname, default_lib_dirpath, lib_dirpath, custom_cachelib_buildpkg_filepath)

        # Assert that liburing (liburing2 and liburing-dev) if exist MUST >= 2.3 for Ubuntu >= 23.04, or NOT exist for Ubuntu <= 20.04 (Facebook's libfolly does NOT support Ubuntu 22.04 with liburing 2.1)
        ## Check if we need liburing
        cachelib_pre_install_tool = None
        need_liburing = False
        check_need_liburing_cmd = "dpkg -l | grep liburing-dev"
        check_need_liburing_subprocess = runCmd(check_need_liburing_cmd)
        if check_need_liburing_subprocess.returncode == 0 and getSubprocessOutputstr(check_need_liburing_subprocess) != "":
            need_liburing = True
        if need_liburing: ## Verify liburing MUST >= 2.3 if need liburing
            need_upgrade_liburing, liburing_old_version = checkVersion(scriptname, "liburing", "2.3", "dpkg -s liburing-dev | grep ^Version: | sed -n 's/^Version: \([0-9\.]\+\)-.*/\\1/p'")
            if need_upgrade_liburing:
                die(scriptname, "liburing {0} is NOT supported by Facebook's libfolly used in CacheLib -> please switch to compatible Ubuntu version (>= 23.04 with liburing >= 2.3 or <= 20.04 without liburing), or purge liburing {0} by dpkg".format(liburing_old_version))

        # Build cachelib and its dependencies
        #cachelib_install_tool = "./contrib/build.sh -j -T -v -S" # For debugging (NOTE: add -S for ./contrib/build.sh to skip git-clone/git-pull step if you have already downloaded external libs required by cachelib in lib/CacheLib/cachelib/external)
        cachelib_install_tool = "./contrib/build.sh -j -T -v"
        #cachelib_pre_install_tool = "sudo apt-get -y install liburing-dev" # Required by libfolly
        installFromRepo(scriptname, cachelib_software_name, cachelib_clone_dirpath, cachelib_install_tool, pre_install_tool = cachelib_pre_install_tool, time_consuming = True)
    else:
        dump(scriptname, "cachelib has already been installed")
    print("")

# (3) Install LRU cache (commit ID: de1c4a0)

# NOTE: we just use LRU downloaded from github in lib/cpp-lru-cache as a reference, while always use our hacked version in src/cache/lru to support required interfaces
if is_install_lrucache:
    lrucache_clone_dirpath = "{}/cpp-lru-cache".format(lib_dirpath)
    lrucache_software_name = "LRU cache"
    lrucache_repo_url = "https://github.com/lamerman/cpp-lru-cache.git"
    cloneRepo(scriptname, lrucache_clone_dirpath, lrucache_software_name, lrucache_repo_url)

    lrucache_target_commit = "de1c4a0"
    checkoutCommit(scriptname, lrucache_clone_dirpath, lrucache_software_name, lrucache_target_commit)
    print("")

# (4) Install LFU cache (commit ID: 0f65db1)

if is_install_lfucache:
    lfucache_clone_dirpath = "{}/caches".format(lib_dirpath)
    lfucache_software_name = "LFU cache"
    lfucache_repo_url = "https://github.com/vpetrigo/caches.git"
    cloneRepo(scriptname, lfucache_clone_dirpath, lfucache_software_name, lfucache_repo_url)

    lfucache_target_commit = "0f65db1"
    checkoutCommit(scriptname, lfucache_clone_dirpath, lfucache_software_name, lfucache_target_commit)
    print("")

# (5) Install RocksDB 8.1.1

if is_install_rocksdb:
    rocksdb_download_filepath="{}/rocksdb-8.1.1.tar.gz".format(lib_dirpath)
    # NOTE: github url needs content-disposition to be converted into the the real url
    rocksdb_download_url = "--content-disposition https://github.com/facebook/rocksdb/archive/refs/tags/v8.1.1.tar.gz"
    downloadTarball(scriptname, rocksdb_download_filepath, rocksdb_download_url)

    rocksdb_decompress_dirpath = "{}/rocksdb-8.1.1".format(lib_dirpath)
    rocksdb_decompress_tool = "tar -xzvf"
    decompressTarball(scriptname, rocksdb_download_filepath, rocksdb_decompress_dirpath, rocksdb_decompress_tool)

    rocksdb_install_dirpath = "{}/librocksdb.a".format(rocksdb_decompress_dirpath)
    rocksdb_install_tool = "PORTABLE=1 make static_lib"
    rocksdb_pre_install_tool = "sudo apt-get -y install libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libjemalloc-dev libsnappy-dev"
    installDecompressedTarball(scriptname, rocksdb_decompress_dirpath, rocksdb_install_dirpath, rocksdb_install_tool, pre_install_tool = rocksdb_pre_install_tool, time_consuming = True)

    if is_clear_tarball:
        clearTarball(scriptname, rocksdb_download_filepath)
    print("")

# (6) Install SMHasher (commit ID: 61a0530)

if is_install_smhasher:
    smhasher_clone_dirpath = "{}/smhasher".format(lib_dirpath)
    smhasher_software_name = "SMHasher"
    smhasher_repo_url = "https://github.com/aappleby/smhasher.git"
    cloneRepo(scriptname, smhasher_clone_dirpath, smhasher_software_name, smhasher_repo_url)

    smhasher_target_commit = "61a0530"
    checkoutCommit(scriptname, smhasher_clone_dirpath, smhasher_software_name, smhasher_target_commit)
    print("")

# (7) Install SegCache (commit ID: 0abdfee)

# NOTE: we just use SegCache downloaded from github in lib/segcache as a reference, while always use our hacked version in src/cache/segcache to fix SegCache's implementation issues
if is_install_segcache:
    segcache_clone_dirpath = "{}/Segcache".format(lib_dirpath)
    segcache_software_name = "SegCache"
    segcache_repo_url = "https://github.com/Thesys-lab/Segcache.git"
    cloneRepo(scriptname, segcache_clone_dirpath, segcache_software_name, segcache_repo_url)

    segcache_target_commit = "0abdfee"
    checkoutCommit(scriptname, segcache_clone_dirpath, segcache_software_name, segcache_target_commit)
    
    # NOTE: use the hacked version to install segcache
    #segcache_install_dirpath = "{}/build".format(segcache_clone_dirpath)
    segcache_install_dirpath = "{}/src/cache/segcache/build".format(proj_dirname)
    tmp_segcache_clone_dirpath = os.path.dirname(segcache_install_dirpath)
    segcache_install_tool = "mkdir build && cd build && cmake .. && make -j"
    installFromRepoIfNot(scriptname, segcache_install_dirpath, segcache_software_name, tmp_segcache_clone_dirpath, segcache_install_tool)
    print("")

# (7) Install GDSF (commit ID: 8818442)

# NOTE: we just use GDSF downloaded from github in lib/webcachesim as a reference, while always use our hacked version in src/cache/greedydual to support required interfaces
if is_install_gdsf:
    gdsf_clone_dirpath = "{}/webcachesim".format(lib_dirpath)
    gdsf_software_name = "GDSF"
    gdsf_repo_url = "https://github.com/dasebe/webcachesim.git"
    cloneRepo(scriptname, gdsf_clone_dirpath, gdsf_software_name, gdsf_repo_url)
    
    gdsf_target_commit = "8818442"
    checkoutCommit(scriptname, gdsf_clone_dirpath, gdsf_software_name, gdsf_target_commit)
    print("")

# (8) Install TommyDS (commit ID: 97ff743)

if is_install_tommyds:
    tommyds_clone_dirpath = "{}/tommyds".format(lib_dirpath)
    tommyds_software_name = "TommyDS"
    tommyds_repo_url = "https://github.com/amadvance/tommyds.git"
    cloneRepo(scriptname, tommyds_clone_dirpath, tommyds_software_name, tommyds_repo_url)

    tommyds_target_commit = "97ff743"
    checkoutCommit(scriptname, tommyds_clone_dirpath, tommyds_software_name, tommyds_target_commit)

    # NOTE: use Makefile of TommyDS itself due to using gcc instead of g++, where gcc does NOT add prefix/suffix to variable names (e.g., functions and classes)
    tommyds_install_dirpath = "{}/libtommy.a".format(tommyds_clone_dirpath)
    tommyds_install_tool = "make all && mv {0}/tommy.o {0}/libtommy.a".format(tommyds_clone_dirpath)
    installFromRepoIfNot(scriptname, tommyds_install_dirpath, tommyds_software_name, tommyds_clone_dirpath, tommyds_install_tool)
    print("")

# (9) Install LHD (commit ID: 806ef46)

if is_install_lhd:
    lhd_clond_dirpath = "{}/lhd".format(lib_dirpath)
    lhd_software_name = "LHD"
    lhd_repo_url = "https://github.com/CMU-CORGI/LHD.git"
    cloneRepo(scriptname, lhd_clond_dirpath, lhd_software_name, lhd_repo_url)

    lhd_target_commit = "806ef46"
    checkoutCommit(scriptname, lhd_clond_dirpath, lhd_software_name, lhd_target_commit)
    print("")

# (10) Others: chown of libraries and update LD_LIBRARY_PATH

## Chown of libraries

prompt(scriptname, "chown of libraries...")
chown_cmd = "sudo chown -R {0}:{0} {1}".format(username, lib_dirpath)
chown_subprocess = runCmd(chown_cmd)
if chown_subprocess.returncode != 0:
    chown_errstr = getSubprocessErrstr(chown_subprocess)
    die(scriptname, "failed to chown of libraries; error: {}".format(chown_errstr))

## Update LD_LIBRARY_PATH

target_ld_libs = ["segcache", "cachelib", "boost", "x86_64-linux-gnu"]
target_ld_lib_dirpaths = ["{}/src/cache/segcache/build/ccommon/lib".format(proj_dirname), "{}/CacheLib/opt/cachelib/lib".format(lib_dirpath), "{}/boost_1_81_0/install/lib".format(lib_dirpath), "/usr/lib/x86_64-linux-gnu"]

prompt(scriptname, "check if need to update LD_LIBRARY_PATH...")
need_update_ld_library_path = False
# NOTE: LD_LIBRARY_PATH will NOT be passed into python interpreter, so we cannot get current_ld_library_path by os.environ.get("LD_LIBRARY_PATH") or runCmd("echo $LD_LIBRARY_PATH")
current_ld_library_path = sys.argv[1]
for tmp_lib in target_ld_libs:
    if tmp_lib not in current_ld_library_path:
        warn(scriptname, "LD_LIBRARY_PATH {} does not contain {}".format(current_ld_library_path, tmp_lib))
        need_update_ld_library_path = True
        break

if need_update_ld_library_path:
    prompt(scriptname, "find bash source filepath...")
    bashrc_source_filepath = os.path.join("/home/{}".format(username), ".bashrc")
    bashprofile_source_filepath = os.path.join("/home/{}".format(username), ".bash_profile")
    bash_source_filepath = ""
    if os.path.exists(bashrc_source_filepath):
        bash_source_filepath = bashrc_source_filepath
    elif os.path.exists(bashprofile_source_filepath):
        bash_source_filepath = bashprofile_source_filepath
    else:
        die(scriptname, "failed to find .bashrc or .bash_profile")

    prompt(scriptname, "check if need to update bash source file {}...".format(bash_source_filepath))
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
            dump(scriptname, "bash source file {} already contains all target libraries".format(bash_source_filepath))
    else: # NOTE: if returncode is not 0, it also means that the target libraries are not contained in LD_LIBRARY_PATH at bash source file
        need_update_bash_source_file = True
        #die(scriptname, "failed to check bash source file {}".format(bash_source_filepath))
    
    if need_update_bash_source_file:
        prompt(scriptname, "update bash source file {}...".format(bash_source_filepath))
        update_bash_source_grepstr = ""
        for i in range(len(target_ld_lib_dirpaths)):
            if i == 0:
                update_bash_source_grepstr = "{}".format(target_ld_lib_dirpaths[i])
            elif i == len(target_ld_lib_dirpaths) - 1:
                update_bash_source_grepstr = "{}:{}:${{LD_LIBRARY_PATH}}".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])
            else:
                update_bash_source_grepstr = "{}:{}".format(update_bash_source_grepstr, target_ld_lib_dirpaths[i])
        update_bash_source_file_cmd = "echo \"export LD_LIBRARY_PATH={}\" >> {}".format(update_bash_source_grepstr, bash_source_filepath)
        update_bash_source_file_subprocess = runCmd(update_bash_source_file_cmd)
        if update_bash_source_file_subprocess.returncode != 0:
            update_bash_source_file_errstr = getSubprocessErrstr(update_bash_source_file_subprocess)
            die(scriptname, "failed to update bash source file {}; error: {}".format(bash_source_filepath, update_bash_source_file_errstr))

    # NOTE: as python will fork a non-interative bash to execute update_ld_library_path_cmd, it cannot find the built-in source command and also cannot change the environment variable of the interative bash launching the python script
    #prompt(scriptname, "source {} to update LD_LIBRARY_PATH...".format(bash_source_filepath))
    #update_ld_library_path_cmd = "source {}".format(bash_source_filepath)
    #update_ld_library_path_subprocess = runCmd(update_ld_library_path_cmd)
    #if update_ld_library_path_subprocess.returncode != 0:
    #    update_ld_library_path_outputstr = getSubprocessErrstr(update_ld_library_path_subprocess)
    #    die(scriptname, "failed to source {} to update LD_LIBRARY_PATH; error: {}".format(bash_source_filepath, update_ld_library_path_outputstr))

    emphasize(scriptname, "Please update LD_LIBRARY_PATH by this command: source {}".format(bash_source_filepath))
else:
    dump(scriptname, "LD_LIBRARY_PATH alreay contains all target libraries")