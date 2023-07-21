#!/usr/bin/env python3

import os
import sys
import subprocess

from paths import *

is_clear_tarball = False # whether to clear intermediate tarball files

# (1) Upgrade python3 if necessary

def versionToTuple(v):
    return tuple(map(int, (v.split("."))))

python3_target_version = "3.7.5"
python3_checkversion_cmd = "python3 --version"
python3_checkversion_subprocess = subprocess.run(python3_checkversion_cmd, shell=True, capture_output=True)
prompt(filename, "check version of python3...")
need_upgrade_python3 = False
if python3_checkversion_subprocess.returncode != 0:
    die(filename, "failed to get the current version of python3")
else:
    python3_checkversion_outputbytes = python3_checkversion_subprocess.stdout
    python3_checkversion_outputstr = python3_checkversion_outputbytes.decode("utf-8")

    python3_current_version_tuple = versionToTuple(python3_checkversion_outputstr)
    python3_target_version_tuple = versionToTuple(python3_target_version)
    if python3_current_version_tuple == python3_target_version_tuple:
        dump(filename, "current python3 version is the same as the target {}".format(python3_target_version))
    elif python3_current_version_tuple > python3_target_version_tuple:
        warn(filename, "current python3 version {} > target version {} (please check if with any issue in subsequent steps)")
    else:
        need_upgrade_python3 = True
        warn(filename, "current python3 version {} < target version {} (need to upgrade python3)")

if need_upgrade_python3:
    python3_download_filepath = "{}/Python-3.7.5.tgz".format(lib_dirpath)
    if not os.path.exists(python3_download_filepath):
        prompt(filename, "download {}...".format(python3_download_filepath))
        python3_download_cmd = "cd {} && wget https://www.python.org/ftp/python/3.7.5/Python-3.7.5.tgz".format(lib_dirpath)

        python3_download_subprocess = subprocess.run(python3_download_cmd, shell=True)
        if python3_download_subprocess.returncode != 0:
            die(filename, "failed to download {}".format(python3_download_filepath))
    else:
        dump(filename, "{} exists (python3.7.5 has been downloaded)".format(python3_download_filepath))

    python3_decompress_dirpath = "{}/Python-3.7.5".format(lib_dirpath)
    if not os.path.exists(python3_decompress_dirpath):
        prompt(filename, "decompress {}...".format(python3_download_filepath))
        python3_decompress_cmd = "cd {} && tar -xvf Python-3.7.5.tgz".format(lib_dirpath)

        python3_decompress_subprocess = subprocess.run(python3_decompress_cmd, shell=True)
        if python3_decompress_subprocess.returncode != 0:
            die(filename, "failed to decompress {}".format(python3_download_filepath))
    else:
        dump(filename, "{} exists (python3.7.5 has been decompressed)".format(python3_decompress_dirpath))

    prompt(filename, "install python3.7.5 from source...")
    python3_install_cmd = "cd {0} && ./configure --enable-optimizations --prefix={1} --exec_prefix={1} && sudo make altinstall && sudo update-alternatives --install {2}/python3 python3 $(readlink -f {2}/python3) 40 && sudo update-alternatives --install {2}/python3 python3 {1}/bin/python3.7 50".format(python3_decompress_dirpath, preferred_dirpath, preferred_binpath)
    python3_install_subprocess = subprocess.run(python3_install_cmd, shell=True)
    if python3_install_subprocess.returncode != 0:
        die(filename, "failed to install python3.7.5")

    if is_clear_tarball:
        warn(filename, "clear {}".format(python3_download_filepath))
        python3_clear_cmd = "cd {} && rm {}".format(lib_dirpath, python3_download_filepath)

        python3_clear_subprocess = subprocess.run(python3_clear_cmd, shell=True)
        if python3_clear_subprocess.returncode != 0:
            die(filename, "failed to clear {}".format(python3_download_filepath))

# (2) Install python libraries (some required by scripts/common.py)

# TODO: END HERE

pylib_requirement_filepath = "scripts/requirements.txt"
#prompt(filename, "install python libraries based on {}...".format(pylib_requirement_filepath))
print("{}: {}".format(filename, "install python libraries based on {}...".format(pylib_requirement_filepath)))
pylib_install_cmd = "python3 -m pip install -r {}".format(pylib_requirement_filepath)

pylib_install_subprocess = subprocess.run(pylib_install_cmd, shell=True)
if pylib_install_subprocess.returncode != 0:
    #die(filename, "failed to install python libraries based on {}".format(pylib_requirement_filepath))
    print("{}: {}".format(filename, "failed to install python libraries based on {}".format(pylib_requirement_filepath)))
    sys.exit(1)

# Include common module for the following installation
from common import *