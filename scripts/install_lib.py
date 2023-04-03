#!/usr/bin/env python3

import os
import sys
import subprocess

from common import *

filename = sys.argv[0]
is_clear = False # whether to clear intermediate files (e.g., tarball files)

lib_dirpath = "lib"
if not os.path.exists(lib_dirpath):
    prompt(filename, "Create directory {}...".format(lib_dirpath))
    os.mkdir(lib_dirpath)
else:
    dump(filename, "{} exists".format(lib_dirpath))

# (1) Install boost 1.81.0

boost_download_filepath = "lib/boost_1_81_0.tar.gz"
if not os.path.exists(boost_download_filepath):
    prompt(filename, "Download {}...".format(boost_download_filepath))
    boost_download_cmd = "cd lib && wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz"

    boost_download_subprocess = subprocess.run(boost_download_cmd, shell=True)
    if boost_download_subprocess.returncode != 0:
        die(filename, "failed to download {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists".format(boost_download_filepath))

boost_decompress_dirpath = "lib/boost_1_81_0"
if not os.path.exists(boost_decompress_dirpath):
    prompt(filename, "Decompress {}...".format(boost_download_filepath))
    boost_decompress_cmd = "cd lib && tar -xzvf boost_1_81_0.tar.gz"

    boost_decompress_subprocess = subprocess.run(boost_decompress_cmd, shell=True)
    if boost_decompress_subprocess.returncode != 0:
        die(filename, "failed to decompress {}".format(boost_download_filepath))
else:
    dump(filename, "{} exists".format(boost_decompress_dirpath))

boost_install_dirpath = "lib/boost_1_81_0/install"
if not os.path.exists(boost_install_dirpath):
    prompt(filename, "Install lib/boost from source...")
    boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test --prefix=./install && sudo ./b2 install".format(boost_decompress_dirpath)
    # Use the following command if your set prefix as /usr/local which needs to update Linux dynamic linker
    #boost_install_cmd = "cd {} && ./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test --prefix=./install && sudo ./b2 install && sudo ldconfig".format(boost_decompress_dirpath)

    boost_install_subprocess = subprocess.run(boost_install_cmd, shell=True)
    if boost_install_subprocess.returncode != 0:
        die(filename, "failed to install {}".format(boost_install_dirpath))
else:
    dump(filename, "{} exists".format(boost_install_dirpath))

if is_clear:
    warn(filename, "clear {}".format(boost_download_filepath))
    boost_clear_cmd = "cd lib && rm {}".format(boost_download_filepath)

    boost_clear_subprocess = subprocess.run(boost_clear_cmd, shell=True)
    if boost_clear_subprocess.returncode != 0:
        die(filename, "failed to clear".format(boost_download_filepath))

# (2) Install python libraries

pylib_requirement_filepath = "scripts/requirements.txt"
prompt(filename, "Install python libraries based on {}...".format(pylib_requirement_filepath))
pylib_install_cmd = "pip3 install -r {}".format(pylib_requirement_filepath)

pylib_install_subprocess = subprocess.run(pylib_install_cmd, shell=True)
if pylib_install_subprocess.returncode != 0:
    die(filename, "failed to install python libraries based on {}".format(pylib_requirement_filepath))