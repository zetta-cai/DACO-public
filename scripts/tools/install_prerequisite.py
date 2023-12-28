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
python3_preferred_binpath = getPreferredDirpathForTarget(scriptname, "python3", system_bin_pathstr) # e.g., /usr/local/bin
python3_installpath = os.path.dirname(python3_preferred_binpath) # e.g., /usr/local
python3_install_binpath = "{}/bin".format(python3_installpath) # e.g., /usr/local/bin

if is_upgrade_python3:
    python3_software_name = "python3"
    python3_target_version = "3.7.5"
    python3_checkversion_cmd = "python3 --version"
    need_upgrade_python3, python3_old_version = checkVersion(scriptname, python3_software_name, python3_target_version, python3_checkversion_cmd)

    if need_upgrade_python3:
        python3_old_canonical_filepath = getCanonicalFilepath(scriptname, python3_software_name, python3_old_version)
        need_preserve_old_python3 = checkOldAlternative(scriptname, python3_software_name, python3_old_canonical_filepath)

        if need_preserve_old_python3:
            preserveOldAlternative(scriptname, python3_software_name, python3_old_canonical_filepath, python3_preferred_binpath)
        else:
            dump(scriptname, "old python3 has been preserved")

        python3_download_filepath = "{}/Python-3.7.5.tgz".format(lib_dirpath)
        python3_download_url = "https://www.python.org/ftp/python/3.7.5/Python-3.7.5.tgz"
        downloadTarball(scriptname, python3_download_filepath, python3_download_url)

        python3_decompress_dirpath = "{}/Python-3.7.5".format(lib_dirpath)
        python3_decompress_tool = "tar -xvf"
        decompressTarball(scriptname, python3_download_filepath, python3_decompress_dirpath, python3_decompress_tool)

        python3_install_filepath = "{}/python3.7".format(python3_install_binpath)
        python3_install_tool = "./configure --enable-optimizations --prefix={0} --exec_prefix={0} && sudo make altinstall".format(python3_installpath)
        installDecompressedTarball(scriptname, python3_decompress_dirpath, python3_install_filepath, python3_install_tool)

        preserveNewAlternative(scriptname, python3_software_name, python3_preferred_binpath, python3_install_filepath)

        if is_clear_tarball:
            clearTarball(scriptname, python3_download_filepath)
    print("")

# (2) Install python libraries (some required by scripts/common.py)

if is_install_pylib:
    pylib_requirement_filepath = "scripts/requirements.txt"
    prompt(scriptname, "install python libraries based on {}...".format(pylib_requirement_filepath))
    pylib_install_cmd = "python3 -m pip install -r {}".format(pylib_requirement_filepath)

    pylib_install_subprocess = runCmd(pylib_install_cmd)
    if pylib_install_subprocess.returncode != 0:
        die(scriptname, "failed to install python libraries based on {}".format(pylib_requirement_filepath))
    print("")

# (3) Upgrade gcc/g++ if necessary

# For gcc/g++
compiler_preferred_binpaths = {}
for compiler_name in ["gcc", "g++"]:
    compiler_preferred_binpaths[compiler_name] = getPreferredDirpathForTarget(scriptname, compiler_name, system_bin_pathstr) # e.g., /usr/local/bin
compiler_installpath = "/usr" # Installed by apt
compiler_install_binpath = "{}/bin".format(compiler_installpath) # /usr/bin

compiler_target_version = "9.4.0"
if is_upgrade_gcc:
    is_add_apt_repo_for_compiler = False
    for compiler_name in ["gcc", "g++"]:
        compiler_checkversion_cmd = "{} --version".format(compiler_name)
        need_upgrade_compiler, compiler_old_version = checkVersion(scriptname, compiler_name, compiler_target_version, compiler_checkversion_cmd)
        
        if need_upgrade_compiler:
            compiler_old_canonical_filepath = getCanonicalFilepath(scriptname, compiler_name, compiler_old_version)
            need_preserve_old_compiler = checkOldAlternative(scriptname, compiler_name, compiler_old_canonical_filepath)
            if need_preserve_old_compiler:
                preserveOldAlternative(scriptname, compiler_name, compiler_old_canonical_filepath, compiler_preferred_binpaths[compiler_name])
            else:
                dump(scriptname, "old {} has been preserved".format(compiler_name))

            # Only need to add once for gcc/g++
            if not is_add_apt_repo_for_compiler:
                prompt(scriptname, "add apt repo for {}...".format(compiler_name))
                compiler_addrepo_cmd = "sudo add-apt-repository ppa:ubuntu-toolchain-r/test && sudo apt update"
                compiler_addrepo_subprocess = runCmd(compiler_addrepo_cmd)
                if compiler_addrepo_subprocess.returncode != 0:
                    die(scriptname, "failed to add apt repo for {}".format(compiler_name))
                is_add_apt_repo_for_compiler = True

            compiler_apt_targetname = "{}-9".format(compiler_name)
            installByApt(scriptname, compiler_name, compiler_apt_targetname)

            compiler_install_filepath = "{}/{}".format(compiler_install_binpath, compiler_apt_targetname)
            preserveNewAlternative(scriptname, compiler_preferred_binpaths[compiler_name], compiler_name, compiler_install_filepath)
    print("")
    
# (4) Link g++ to c++ for cachelib to compile libfolly

if is_link_cpp:
    cpp_software_name = "c++"
    cpp_checkversion_cmd = "c++ --version"
    need_link_cpp, _ = checkVersion(scriptname, cpp_software_name, compiler_target_version, cpp_checkversion_cmd)

    if need_link_cpp:
        prompt(scriptname, "link g++-9 to c++ binary...")
        link_cpp_cmd = "sudo mv $(which c++) $(which c++).bak; sudo ln -s {0}/g++ {0}/c++".foramt(compiler_preferred_binpaths["g++"])
        link_cpp_subprocess = runCmd(link_cpp_cmd)
        if link_cpp_subprocess.returncode != 0:
            die(scriptname, "failed to link g++-9 to c++ binary")
    print("")

# (5) Upgrade CMake if necessary (required by cachelib for CMake 3.12 or higher)

if is_upgrade_cmake:
    cmake_software_name = "cmake"
    cmake_target_version = "3.25.2"
    cmake_checkversion_cmd = "cmake --version"
    need_upgrade_cmake, cmake_old_version = checkVersion(scriptname, cmake_software_name, cmake_target_version, cmake_checkversion_cmd)

    if need_upgrade_cmake:
        cmake_installpath = "/usr" # Installed by apt
        cmake_install_binpath = "{}/bin".format(cmake_installpath) # /usr/bin
        cmake_preferred_binpath = getPreferredDirpathForTarget(scriptname, cmake_software_name, system_bin_pathstr) # e.g., /usr/local/bin

        cmake_old_canonical_filepath = getCanonicalFilepath(scriptname, cmake_software_name, cmake_old_version)
        need_preserve_old_cmake = checkOldAlternative(scriptname, cmake_software_name, cmake_old_canonical_filepath)
        if need_preserve_old_cmake:
            preserveOldAlternative(scriptname, cmake_software_name, cmake_old_canonical_filepath, cmake_preferred_binpath)
        else:
            dump(scriptname, "old {} has been preserved".format(cmake_software_name))

        prompt(scriptname, "check apt repo for CMake...")
        is_add_apt_repo_for_cmake = True
        cmake_repo_website = "https://apt.kitware.com/ubuntu"
        cmake_check_apt_repo_cmd = "sudo grep ^ /etc/apt/sources.list /etc/apt/sources.list.d/* | grep {}".format(cmake_repo_website)
        cmake_check_apt_repo_subprocess = runCmd(cmake_check_apt_repo_cmd)
        if cmake_check_apt_repo_subprocess.returncode != 0:
            cmake_check_apt_repo_errstr = getSubprocessErrstr(cmake_check_apt_repo_subprocess)
            if cmake_check_apt_repo_errstr != "":
                die(scriptname, "failed to check apt repo for CMake (errmsg: {})".format(cmake_check_apt_repo_errstr))
            else:
                is_add_apt_repo_for_cmake = True
        else:
            cmake_check_apt_repo_outputstr = getSubprocessOutputstr(cmake_check_apt_repo_subprocess)
            if cmake_repo_website in cmake_check_apt_repo_outputstr:
                is_add_apt_repo_for_cmake = False
            else:
                is_add_apt_repo_for_cmake = True

        if is_add_apt_repo_for_cmake:
            prompt(scriptname, "Add apt repo for CMake...")
            cmake_add_apt_repo_cmd = "wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add - && sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ {} main' && sudo apt-get update && sudo apt-get install kitware-archive-keyring && sudo apt-key --keyring /etc/apt/trusted.gpg del C1F34CDD40CD72DA".format(kernel_codename)
            cmake_add_apt_repo_subprocess = runCmd(cmake_add_apt_repo_cmd, is_capture_output=False)
            if cmake_add_apt_repo_subprocess.returncode != 0:
                die(scriptname, "failed to add apt repo for CMake")
        
        cmake_apt_targetname = "cmake"
        installByApt(scriptname, cmake_software_name, cmake_apt_targetname)

        # NOTE: as cmake apt target name is the same as cmake software name, we have to get new canonical filepath for cmake to preserve new alternative
        tmp_need_upgrade_cmake, cmake_new_version = checkVersion(scriptname, cmake_software_name, cmake_target_version, cmake_checkversion_cmd)
        if tmp_need_upgrade_cmake:
            die(scriptname, "cmake {} has NOT been upgraded to >= {} successfully!".format(cmake_new_version, cmake_target_version))
        cmake_new_canonical_filepath = getCanonicalFilepath(scriptname, cmake_software_name, cmake_new_version) # /usr/bin/cmake-3.25.2
        preserveNewAlternative(scriptname, cmake_preferred_binpath, cmake_software_name, cmake_new_canonical_filepath)
    print("")

# (6) Change system settings

prompt(scriptname, "check net.core.rmem_max...")
target_rmem_max = 16777216
need_set_rmem_max = False
check_rmem_max_cmd = "sysctl -a 2>/dev/null | grep net.core.rmem_max"
check_rmem_max_subprocess = runCmd(check_rmem_max_cmd)
if check_rmem_max_subprocess.returncode != 0:
    die(scriptname, "failed to check net.core.rmem_max")
else:
    check_rmem_max_outputstr = getSubprocessOutputstr(check_rmem_max_subprocess)
    cur_rmem_max = int(check_rmem_max_outputstr.split("=")[-1])
    if cur_rmem_max < target_rmem_max:
        dump(scriptname, "current net.core.rmem_max {} < target {}, which needs reset".format(cur_rmem_max, target_rmem_max))
        need_set_rmem_max = True
    else:
        dump(scriptname, "current net.core.rmem_max {} already >= target {}, which does not need reset".format(cur_rmem_max, target_rmem_max))

if need_set_rmem_max:
    prompt(scriptname, "set net.core.rmem_max as {}...".format(target_rmem_max))
    set_rmem_max_cmd = "sudo sysctl -w net.core.rmem_max={}".format(target_rmem_max)
    set_rmem_max_subprocess = runCmd(set_rmem_max_cmd)
    if set_rmem_max_subprocess.returncode != 0:
        die(scriptname, "failed to set net.core.rmem_max")