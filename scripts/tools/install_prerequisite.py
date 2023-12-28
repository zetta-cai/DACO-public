#!/usr/bin/env python3
# install_prerequisite: install pre-requisites automatically

import os
import sys

from ..common import *

is_clear_tarball = False # whether to clear intermediate tarball files

# Variables to control whether to install the corresponding softwares
is_upgrade_python3 = True
is_install_pylib = True
is_upgrade_gcc = True
is_link_cpp = True
is_upgrade_cmake = True

# (1) Upgrade python3 if necessary

# For python3
python3_preferred_binpath = PathUtil.getPreferredDirpathForTarget(Common.scriptname, "python3") # e.g., /usr/local/bin
python3_installpath = os.path.dirname(python3_preferred_binpath) # e.g., /usr/local
python3_install_binpath = "{}/bin".format(python3_installpath) # e.g., /usr/local/bin

if is_upgrade_python3:
    python3_software_name = "python3"
    python3_target_version = "3.7.5"
    python3_checkversion_cmd = "python3 --version"
    need_upgrade_python3, python3_old_version = SubprocessUtil.checkVersion(Common.scriptname, python3_software_name, python3_target_version, python3_checkversion_cmd)

    if need_upgrade_python3:
        python3_old_canonical_filepath = SubprocessUtil.getCanonicalFilepath(Common.scriptname, python3_software_name, python3_old_version)
        need_preserve_old_python3 = SubprocessUtil.checkOldAlternative(Common.scriptname, python3_software_name, python3_old_canonical_filepath)

        if need_preserve_old_python3:
            SubprocessUtil.preserveOldAlternative(Common.scriptname, python3_software_name, python3_old_canonical_filepath, python3_preferred_binpath)
        else:
            LogUtil.dump(Common.scriptname, "old python3 has been preserved")

        python3_download_filepath = "{}/Python-3.7.5.tgz".format(Common.lib_dirpath)
        python3_download_url = "https://www.python.org/ftp/python/3.7.5/Python-3.7.5.tgz"
        SubprocessUtil.downloadTarball(Common.scriptname, python3_download_filepath, python3_download_url)

        python3_decompress_dirpath = "{}/Python-3.7.5".format(Common.lib_dirpath)
        python3_decompress_tool = "tar -xvf"
        SubprocessUtil.decompressTarball(Common.scriptname, python3_download_filepath, python3_decompress_dirpath, python3_decompress_tool)

        python3_install_filepath = "{}/python3.7".format(python3_install_binpath)
        python3_install_tool = "./configure --enable-optimizations --prefix={0} --exec_prefix={0} && sudo make altinstall".format(python3_installpath)
        SubprocessUtil.installDecompressedTarball(Common.scriptname, python3_decompress_dirpath, python3_install_filepath, python3_install_tool)

        SubprocessUtil.preserveNewAlternative(Common.scriptname, python3_software_name, python3_preferred_binpath, python3_install_filepath)

        if is_clear_tarball:
            SubprocessUtil.clearTarball(Common.scriptname, python3_download_filepath)
    print("")

# (2) Install python libraries (some required by scripts/common.py)

if is_install_pylib:
    pylib_requirement_filepath = "scripts/requirements.txt"
    LogUtil.prompt(Common.scriptname, "install python libraries based on {}...".format(pylib_requirement_filepath))
    pylib_install_cmd = "python3 -m pip install -r {}".format(pylib_requirement_filepath)

    pylib_install_subprocess = SubprocessUtil.runCmd(pylib_install_cmd)
    if pylib_install_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to install python libraries based on {}".format(pylib_requirement_filepath))
    print("")

# (3) Upgrade gcc/g++ if necessary

# For gcc/g++
compiler_preferred_binpaths = {}
for compiler_name in ["gcc", "g++"]:
    compiler_preferred_binpaths[compiler_name] = PathUtil.getPreferredDirpathForTarget(Common.scriptname, compiler_name) # e.g., /usr/local/bin
compiler_installpath = "/usr" # Installed by apt
compiler_install_binpath = "{}/bin".format(compiler_installpath) # /usr/bin

compiler_target_version = "9.4.0"
if is_upgrade_gcc:
    is_add_apt_repo_for_compiler = False
    for compiler_name in ["gcc", "g++"]:
        compiler_checkversion_cmd = "{} --version".format(compiler_name)
        need_upgrade_compiler, compiler_old_version = SubprocessUtil.checkVersion(Common.scriptname, compiler_name, compiler_target_version, compiler_checkversion_cmd)
        
        if need_upgrade_compiler:
            compiler_old_canonical_filepath = SubprocessUtil.getCanonicalFilepath(Common.scriptname, compiler_name, compiler_old_version)
            need_preserve_old_compiler = SubprocessUtil.checkOldAlternative(Common.scriptname, compiler_name, compiler_old_canonical_filepath)
            if need_preserve_old_compiler:
                SubprocessUtil.preserveOldAlternative(Common.scriptname, compiler_name, compiler_old_canonical_filepath, compiler_preferred_binpaths[compiler_name])
            else:
                LogUtil.dump(Common.scriptname, "old {} has been preserved".format(compiler_name))

            # Only need to add once for gcc/g++
            if not is_add_apt_repo_for_compiler:
                LogUtil.prompt(Common.scriptname, "add apt repo for {}...".format(compiler_name))
                compiler_addrepo_cmd = "sudo add-apt-repository ppa:ubuntu-toolchain-r/test && sudo apt update"
                compiler_addrepo_subprocess = SubprocessUtil.runCmd(compiler_addrepo_cmd)
                if compiler_addrepo_subprocess.returncode != 0:
                    LogUtil.die(Common.scriptname, "failed to add apt repo for {}".format(compiler_name))
                is_add_apt_repo_for_compiler = True

            compiler_apt_targetname = "{}-9".format(compiler_name)
            SubprocessUtil.installByApt(Common.scriptname, compiler_name, compiler_apt_targetname)

            compiler_install_filepath = "{}/{}".format(compiler_install_binpath, compiler_apt_targetname)
            SubprocessUtil.preserveNewAlternative(Common.scriptname, compiler_preferred_binpaths[compiler_name], compiler_name, compiler_install_filepath)
    print("")
    
# (4) Link g++ to c++ for cachelib to compile libfolly

if is_link_cpp:
    cpp_software_name = "c++"
    cpp_checkversion_cmd = "c++ --version"
    need_link_cpp, _ = SubprocessUtil.checkVersion(Common.scriptname, cpp_software_name, compiler_target_version, cpp_checkversion_cmd)

    if need_link_cpp:
        LogUtil.prompt(Common.scriptname, "link g++-9 to c++ binary...")
        link_cpp_cmd = "sudo mv $(which c++) $(which c++).bak; sudo ln -s {0}/g++ {0}/c++".foramt(compiler_preferred_binpaths["g++"])
        link_cpp_subprocess = SubprocessUtil.runCmd(link_cpp_cmd)
        if link_cpp_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, "failed to link g++-9 to c++ binary")
    print("")

# (5) Upgrade CMake if necessary (required by cachelib for CMake 3.12 or higher)

if is_upgrade_cmake:
    cmake_software_name = "cmake"
    cmake_target_version = "3.25.2"
    cmake_checkversion_cmd = "cmake --version"
    need_upgrade_cmake, cmake_old_version = SubprocessUtil.checkVersion(Common.scriptname, cmake_software_name, cmake_target_version, cmake_checkversion_cmd)

    if need_upgrade_cmake:
        cmake_installpath = "/usr" # Installed by apt
        cmake_install_binpath = "{}/bin".format(cmake_installpath) # /usr/bin
        cmake_preferred_binpath = PathUtil.getPreferredDirpathForTarget(Common.scriptname, cmake_software_name) # e.g., /usr/local/bin

        cmake_old_canonical_filepath = SubprocessUtil.getCanonicalFilepath(Common.scriptname, cmake_software_name, cmake_old_version)
        need_preserve_old_cmake = SubprocessUtil.checkOldAlternative(Common.scriptname, cmake_software_name, cmake_old_canonical_filepath)
        if need_preserve_old_cmake:
            SubprocessUtil.preserveOldAlternative(Common.scriptname, cmake_software_name, cmake_old_canonical_filepath, cmake_preferred_binpath)
        else:
            LogUtil.dump(Common.scriptname, "old {} has been preserved".format(cmake_software_name))

        LogUtil.prompt(Common.scriptname, "check apt repo for CMake...")
        is_add_apt_repo_for_cmake = True
        cmake_repo_website = "https://apt.kitware.com/ubuntu"
        cmake_check_apt_repo_cmd = "sudo grep ^ /etc/apt/sources.list /etc/apt/sources.list.d/* | grep {}".format(cmake_repo_website)
        cmake_check_apt_repo_subprocess = SubprocessUtil.runCmd(cmake_check_apt_repo_cmd)
        if cmake_check_apt_repo_subprocess.returncode != 0:
            cmake_check_apt_repo_errstr = SubprocessUtil.getSubprocessErrstr(cmake_check_apt_repo_subprocess)
            if cmake_check_apt_repo_errstr != "":
                LogUtil.die(Common.scriptname, "failed to check apt repo for CMake (errmsg: {})".format(cmake_check_apt_repo_errstr))
            else:
                is_add_apt_repo_for_cmake = True
        else:
            cmake_check_apt_repo_outputstr = SubprocessUtil.getSubprocessOutputstr(cmake_check_apt_repo_subprocess)
            if cmake_repo_website in cmake_check_apt_repo_outputstr:
                is_add_apt_repo_for_cmake = False
            else:
                is_add_apt_repo_for_cmake = True

        if is_add_apt_repo_for_cmake:
            LogUtil.prompt(Common.scriptname, "Add apt repo for CMake...")
            cmake_add_apt_repo_cmd = "wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add - && sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ {} main' && sudo apt-get update && sudo apt-get install kitware-archive-keyring && sudo apt-key --keyring /etc/apt/trusted.gpg del C1F34CDD40CD72DA".format(Common.kernel_codename)
            cmake_add_apt_repo_subprocess = SubprocessUtil.runCmd(cmake_add_apt_repo_cmd, is_capture_output=False)
            if cmake_add_apt_repo_subprocess.returncode != 0:
                LogUtil.die(Common.scriptname, "failed to add apt repo for CMake")
        
        cmake_apt_targetname = "cmake"
        SubprocessUtil.installByApt(Common.scriptname, cmake_software_name, cmake_apt_targetname)

        # NOTE: as cmake apt target name is the same as cmake software name, we have to get new canonical filepath for cmake to preserve new alternative
        tmp_need_upgrade_cmake, cmake_new_version = SubprocessUtil.checkVersion(Common.scriptname, cmake_software_name, cmake_target_version, cmake_checkversion_cmd)
        if tmp_need_upgrade_cmake:
            LogUtil.die(Common.scriptname, "cmake {} has NOT been upgraded to >= {} successfully!".format(cmake_new_version, cmake_target_version))
        cmake_new_canonical_filepath = SubprocessUtil.getCanonicalFilepath(Common.scriptname, cmake_software_name, cmake_new_version) # /usr/bin/cmake-3.25.2
        SubprocessUtil.preserveNewAlternative(Common.scriptname, cmake_preferred_binpath, cmake_software_name, cmake_new_canonical_filepath)
    print("")

# (6) Change system settings

LogUtil.prompt(Common.scriptname, "check net.core.rmem_max...")
target_rmem_max = 16777216
need_set_rmem_max = False
check_rmem_max_cmd = "sysctl -a 2>/dev/null | grep net.core.rmem_max"
check_rmem_max_subprocess = SubprocessUtil.runCmd(check_rmem_max_cmd)
if check_rmem_max_subprocess.returncode != 0:
    LogUtil.die(Common.scriptname, "failed to check net.core.rmem_max")
else:
    check_rmem_max_outputstr = SubprocessUtil.getSubprocessOutputstr(check_rmem_max_subprocess)
    cur_rmem_max = int(check_rmem_max_outputstr.split("=")[-1])
    if cur_rmem_max < target_rmem_max:
        LogUtil.dump(Common.scriptname, "current net.core.rmem_max {} < target {}, which needs reset".format(cur_rmem_max, target_rmem_max))
        need_set_rmem_max = True
    else:
        LogUtil.dump(Common.scriptname, "current net.core.rmem_max {} already >= target {}, which does not need reset".format(cur_rmem_max, target_rmem_max))

if need_set_rmem_max:
    LogUtil.prompt(Common.scriptname, "set net.core.rmem_max as {}...".format(target_rmem_max))
    set_rmem_max_cmd = "sudo sysctl -w net.core.rmem_max={}".format(target_rmem_max)
    set_rmem_max_subprocess = SubprocessUtil.runCmd(set_rmem_max_cmd)
    if set_rmem_max_subprocess.returncode != 0:
        LogUtil.die(Common.scriptname, "failed to set net.core.rmem_max")