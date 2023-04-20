#!/usr/bin/env python3

import os
import sys
import subprocess

filename = sys.argv[0]
is_clear = False # whether to clear intermediate files (e.g., tarball files)

# (1) Install python libraries (some required by scripts/common.py)

pylib_requirement_filepath = "scripts/requirements.txt"
#prompt(filename, "install python libraries based on {}...".format(pylib_requirement_filepath))
print("{}: {}".format(filename, "install python libraries based on {}...".format(pylib_requirement_filepath)))
pylib_install_cmd = "pip3 install -r {}".format(pylib_requirement_filepath)

pylib_install_subprocess = subprocess.run(pylib_install_cmd, shell=True)
if pylib_install_subprocess.returncode != 0:
    #die(filename, "failed to install python libraries based on {}".format(pylib_requirement_filepath))
    print("{}: {}".format(filename, "failed to install python libraries based on {}".format(pylib_requirement_filepath)))
    sys.exit(1)

# Include common module for the following installation
from common import *

lib_dirpath = "lib"
if not os.path.exists(lib_dirpath):
    prompt(filename, "Create directory {}...".format(lib_dirpath))
    os.mkdir(lib_dirpath)
else:
    dump(filename, "{} exists (directory has been created)".format(lib_dirpath))

# (2) Install boost 1.81.0

boost_download_filepath = "{}/boost_1_81_0.tar.gz".format(lib_dirpath)
if not os.path.exists(boost_download_filepath):
    prompt(filename, "download {}...".format(boost_download_filepath))
    boost_download_cmd = "cd lib && wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz"

    boost_download_subprocess = subprocess.run(boost_download_cmd, shell=True)
    if boost_download_subprocess.returncode != 0:
        die(filename, "failed to download {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists (boost has been downloaded)".format(boost_download_filepath))

boost_decompress_dirpath = "{}/boost_1_81_0".format(lib_dirpath)
if not os.path.exists(boost_decompress_dirpath):
    prompt(filename, "decompress {}...".format(boost_download_filepath))
    boost_decompress_cmd = "cd lib && tar -xzvf boost_1_81_0.tar.gz"

    boost_decompress_subprocess = subprocess.run(boost_decompress_cmd, shell=True)
    if boost_decompress_subprocess.returncode != 0:
        die(filename, "failed to decompress {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists (boost has been decompressed)".format(boost_decompress_dirpath))

boost_install_dirpath = "{}/install".format(boost_decompress_dirpath)
if not os.path.exists(boost_install_dirpath):
    prompt(filename, "install lib/boost from source...")
    boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test,json --prefix=./install && sudo ./b2 install".format(boost_decompress_dirpath)
    # Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
    #boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test --prefix=./install && sudo ./b2 install && sudo ldconfig".format(boost_decompress_dirpath)

    boost_install_subprocess = subprocess.run(boost_install_cmd, shell=True)
    if boost_install_subprocess.returncode != 0:
        die(filename, "failed to install {}".format(boost_install_dirpath))
else:
    dump(filename, "{} exists (boost has been installed)".format(boost_install_dirpath))

if is_clear:
    warn(filename, "clear {}".format(boost_download_filepath))
    boost_clear_cmd = "cd lib && rm {}".format(boost_download_filepath)

    boost_clear_subprocess = subprocess.run(boost_clear_cmd, shell=True)
    if boost_clear_subprocess.returncode != 0:
        die(filename, "failed to clear".format(boost_download_filepath))

# (3) Install cachelib (commit ID: 3d475f6)

cachelib_clone_dirpath = "{}/CacheLib".format(lib_dirpath)
if not os.path.exists(cachelib_clone_dirpath):
    prompt(filename, "clone cachelib into {}...".format(cachelib_clone_dirpath))
    cachelib_clone_cmd = "cd lib && git clone https://github.com/facebook/CacheLib.git"

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