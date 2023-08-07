#!/usr/bin/env python3

import os
import sys
import subprocess

from paths import *

is_clear_tarball = False # whether to clear intermediate tarball files

def versionToTuple(v):
    return tuple(map(int, (v.split("."))))

# Variables to control whether to install the corresponding softwares
is_upgrade_python3 = True
is_install_pylib = True
is_upgrade_gcc = True
is_link_cpp = True
is_upgrade_cmake = True

# (1) Upgrade python3 if necessary

if is_upgrade_python3:
    print("{}: check version of python3...".format(filename))
    python3_target_version = "3.7.5"
    python3_checkversion_cmd = "python3 --version"
    python3_checkversion_subprocess = subprocess.run(python3_checkversion_cmd, shell=True, capture_output=True)
    need_upgrade_python3 = False
    if python3_checkversion_subprocess.returncode != 0:
        print("{}: failed to get the current version of python3".format(filename))
        sys.exit(1)
    else:
        python3_checkversion_outputbytes = python3_checkversion_subprocess.stdout
        python3_checkversion_outputstr = python3_checkversion_outputbytes.decode("utf-8")

        python3_current_version_tuple = versionToTuple(python3_checkversion_outputstr)
        python3_target_version_tuple = versionToTuple(python3_target_version)
        if python3_current_version_tuple == python3_target_version_tuple:
            print("{}: current python3 version is the same as the target {}".format(filename, python3_target_version))
        elif python3_current_version_tuple > python3_target_version_tuple:
            print("{}: current python3 version {} > target version {} (please check if with any issue in subsequent steps)".format(filename, python3_checkversion_outputstr, python3_target_version))
        else:
            need_upgrade_python3 = True
            print("{}: current python3 version {} < target version {} (need to upgrade python3)".format(filename, python3_checkversion_outputstr, python3_target_version))

    if need_upgrade_python3:
        print("{}: check if old python3 is preserved...".format(filename))
        python3_check_old_cmd = "sudo update-alternatives --query python3 | grep $(readlink -f $(which python3))"
        python3_check_old_subprocess = subprocess.run(python3_check_old_cmd, shell=True, capture_output=True)
        need_preserve_old_python3 = True;
        if python3_check_old_subprocess.returncode == 0:
            python3_check_old_outputbytes = python3_check_old_subprocess.stdout
            python3_check_old_outputstr = python3_check_old_outputbytes.decode("utf-8")
            if python3_check_old_outputstr != "":
                need_preserve_old_python3 = False

        if need_preserve_old_python3:
            print("{}: preserve old python3...".format(filename))
            python3_preserve_old_cmd = "sudo update-alternatives --install {}/python3 python3 $(readlink -f $(which python3) 40".format(preferred_binpath)
            python3_preserve_old_subprocess = subprocess.run(python3_preserve_old_cmd, shell=True)
            if python3_preserve_old_subprocess.returncode != 0:
                print("{}: failed to preserve old python3".format(filename))
                sys.exit(1)
        else:
            print("{}: old python3 has been preserved".format(filename))

        python3_download_filepath = "{}/Python-3.7.5.tgz".format(lib_dirpath)
        if not os.path.exists(python3_download_filepath):
            print("{}: download {}...".format(filename, python3_download_filepath))
            python3_download_cmd = "cd {} && wget https://www.python.org/ftp/python/3.7.5/Python-3.7.5.tgz".format(lib_dirpath)

            python3_download_subprocess = subprocess.run(python3_download_cmd, shell=True)
            if python3_download_subprocess.returncode != 0:
                print("{}: failed to download {}".format(filename, python3_download_filepath))
                sys.exit(1)
        else:
            print("{}: {} exists (python3.7.5 has been downloaded)".format(filename, python3_download_filepath))

        python3_decompress_dirpath = "{}/Python-3.7.5".format(lib_dirpath)
        if not os.path.exists(python3_decompress_dirpath):
            print("{}: decompress {}...".format(filename, python3_download_filepath))
            python3_decompress_cmd = "cd {} && tar -xvf Python-3.7.5.tgz".format(lib_dirpath)

            python3_decompress_subprocess = subprocess.run(python3_decompress_cmd, shell=True)
            if python3_decompress_subprocess.returncode != 0:
                print("{}: failed to decompress {}".format(filename, python3_download_filepath))
                sys.exit(1)
        else:
            print("{}: {} exists (python3.7.5 has been decompressed)".format(filename, python3_decompress_dirpath))

        python3_install_filepath = "{}/python3.7".format(custom_binpath)
        if not os.path.exists(python3_install_filepath):
            print("{}: install python3.7.5 from source...".format(filename))
            python3_install_cmd = "cd {0} && ./configure --enable-optimizations --prefix={1} --exec_prefix={1} && sudo make altinstall".format(python3_decompress_dirpath, custom_installpath)
            python3_install_subprocess = subprocess.run(python3_install_cmd, shell=True)
            if python3_install_subprocess.returncode != 0:
                print("{}: failed to install python3.7.5".format(filename))
                sys.exit(1)
        else:
            print("{}: {} exists (python3.7.5 has been installed)".format(filename, python3_install_filepath))

        print("{}: switch to python3.7.5...".format(filename))
        python3_switch_new_cmd = "sudo update-alternatives --install {}/python3 python3 {}/python3.7 50".format(preferred_binpath, custom_binpath)
        python3_switch_new_subprocess = subprocess.run(python3_switch_new_cmd, shell=True)
        if python3_switch_new_subprocess.returncode != 0:
            print("{}: failed to switch python3.7.5".format(filename))
            sys.exit(1)

        if is_clear_tarball:
            print("{}: clear {}".format(filename, python3_download_filepath))
            python3_clear_cmd = "cd {} && rm {}".format(lib_dirpath, python3_download_filepath)

            python3_clear_subprocess = subprocess.run(python3_clear_cmd, shell=True)
            if python3_clear_subprocess.returncode != 0:
                print("{}: failed to clear {}".format(filename, python3_download_filepath))
                sys.exit(1)

# (2) Install python libraries (some required by scripts/common.py)

if is_install_pylib:
    pylib_requirement_filepath = "scripts/requirements.txt"
    print("{}: install python libraries based on {}...".format(filename, pylib_requirement_filepath))
    pylib_install_cmd = "python3 -m pip install -r {}".format(pylib_requirement_filepath)

    pylib_install_subprocess = subprocess.run(pylib_install_cmd, shell=True)
    if pylib_install_subprocess.returncode != 0:
        print("{}: failed to install python libraries based on {}".format(filename, pylib_requirement_filepath))
        sys.exit(1)

# Include common module for the following installation
from common import *

# (3) Upgrade gcc/g++ if necessary

if is_upgrade_gcc:
    compiler_target_version = "9.4.0"
    is_add_apt_repo_for_compiler = False
    for compiler_name in ["gcc", "g++"]:
        prompt(filename, "check version of {}...".format(compiler_name))
        compiler_checkversion_cmd = "{} --version".format(compiler_name)
        compiler_checkversion_subprocess = subprocess.run(compiler_checkversion_cmd, shell=True, capture_output=True)
        need_upgrade_compiler = False
        if compiler_checkversion_subprocess.returncode != 0:
            die(filename, "failed to get the current version of {}".format(compiler_name))
        else:
            compiler_checkversion_outputbytes = compiler_checkversion_subprocess.stdout
            compiler_checkversion_outputstr = compiler_checkversion_outputbytes.decode("utf-8")

            compiler_current_version_tuple = versionToTuple(compiler_checkversion_outputstr)
            compiler_target_version_tuple = versionToTuple(compiler_target_version)
            if compiler_current_version_tuple == compiler_target_version_tuple:
                dump(filename, "current {} version is the same as the target {}".format(compiler_name, compiler_target_version))
            elif compiler_current_version_tuple > compiler_target_version_tuple:
                warn(filename, "current {} version {} > target version {} (please check if with any issue in subsequent steps)".format(compiler_name, compiler_checkversion_outputstr, compiler_target_version))
            else:
                need_upgrade_compiler = True
                warn(filename, "current {} version {} < target version {} (need to upgrade {})".format(compiler_name, compiler_checkversion_outputstr, compiler_target_version, compiler_name))
        
        if need_upgrade_compiler:
            prompt(filename, "check if old {} is preserved...".format(compiler_name))
            compiler_check_old_cmd = "sudo update-alternatives --query {0} | grep $(readlink -f $(which {0}))".format(compiler_name)
            compiler_check_old_subprocess = subprocess.run(compiler_check_old_cmd, shell=True, capture_output=True)
            need_preserve_old_compiler = True;
            if compiler_check_old_subprocess.returncode == 0:
                compiler_check_old_outputbytes = compiler_check_old_subprocess.stdout
                compiler_check_old_outputstr = compiler_check_old_outputbytes.decode("utf-8")
                if compiler_check_old_outputstr != "":
                    need_preserve_old_compiler = False

            if need_preserve_old_compiler:
                prompt(filename, "preserve old {}...".format(compiler_name))
                compiler_preserve_old_cmd = "sudo update-alternatives --install {0}/{1} {1} $(readlink -f $(which {1})) 40".format(preferred_binpath, compiler_name)
                compiler_preserve_old_subprocess = subprocess.run(compiler_preserve_old_cmd, shell=True)
                if compiler_preserve_old_subprocess.returncode != 0:
                    die(filename, "failed to preserve old {}".format(compiler_name))
            else:
                dump(filename, "old {} has been preserved".format(compiler_name))

            # Only need to add once for gcc/g++
            if not is_add_apt_repo_for_compiler:
                prompt(filename, "add apt repo for {}...".format(compiler_name))
                compiler_addrepo_cmd = "sudo add-apt-repository ppa:ubuntu-toolchain-r/test && sudo apt update"
                compiler_addrepo_subprocess = subprocess.run(compiler_addrepo_cmd, shell=True)
                if compiler_addrepo_subprocess.returncode != 0:
                    die(filename, "failed to add apt repo for {}".format(compiler_name))
                is_add_apt_repo_for_compiler = True

            prompt(filename, "install {} by apt...".format(compiler_name))
            compiler_install_cmd = "sudo apt install {}-9".format(compiler_name)
            compiler_install_subprocess = subprocess.run(compiler_install_cmd, shell=True)
            if compiler_install_subprocess.returncode != 0:
                die(filename, "failed to install {}-9".format(compiler_name))

            prompt(filename, "switch to {}-9...".format(compiler_name))
            compiler_switch_cmd = "sudo update-alternatives --install {0}/{1} {1} {2}/{1}-9 50".format(preferred_binpath, compiler_name, compiler_binpath)
            compiler_switch_subprocess = subprocess.run(compiler_switch_cmd, shell=True)
            if compiler_switch_subprocess.returncode != 0:
                die(filename, "failed to switch {}-9".format(compiler_name))
    
# (4) Link g++ to c++ for cachelib to compile libfolly

if is_link_cpp:
    prompt(filename, "check version of c++ binary...")
    cpp_checkversion_cmd = "c++ --version"
    cpp_checkversion_subprocess = subprocess.run(cpp_checkversion_cmd, shell=True, capture_output=True)
    need_link_cpp = True
    if cpp_checkversion_subprocess.returncode == 0:
        cpp_checkversion_outputbytes = cpp_checkversion_subprocess.stdout
        cpp_checkversion_outputstr = cpp_checkversion_outputbytes.decode("utf-8")

        cpp_current_version_tuple = versionToTuple(cpp_checkversion_outputstr)
        cpp_target_version_tuple = versionToTuple(compiler_target_version)
        if cpp_current_version_tuple == cpp_target_version_tuple:
            dump(filename, "current c++ binary version is the same as the target {}".format(compiler_target_version))
            need_link_cpp = False
        elif cpp_current_version_tuple > cpp_target_version_tuple:
            warn(filename, "current c++ binary version {} > target version {} (please check if with any issue in subsequent steps)".format(cpp_checkversion_outputstr, compiler_target_version))
            need_link_cpp = False
        else:
            warn(filename, "current c++ binary version {} < target version {} (need to link c++ binary to g++-9)".format(cpp_checkversion_outputstr, compiler_target_version))

    if need_link_cpp:
        prompt(filename, "link g++-9 to c++ binary...")
        link_cpp_cmd = "sudo mv $(which c++) $(which c++).bak; sudo ln -s {0}/g++ {0}/c++".foramt(preferred_binpath)
        link_cpp_subprocess = subprocess.run(link_cpp_cmd, shell=True)
        if link_cpp_subprocess.returncode != 0:
            die(filename, "failed to link g++-9 to c++ binary")

# (5) Upgrade CMake if necessary (required by cachelib for CMake 3.12 or higher)

if is_upgrade_cmake:
    prompt(fiename, "check version of CMake...")
    need_upgrade_cmake = False
    cmake_target_version = "3.25.2"
    cmake_checkversion_cmd = "cmake --version"
    cmake_checkversion_subprocess = subprocess.run(cmake_checkversion_cmd, shell=True, capture_output=True)
    if cmake_checkversion_subprocess.returncode != 0:
        die(filename, "failed to get the current version of cmake")
    else:
        cmake_checkversion_outputbytes = cmake_checkversion_subprocess.stdout
        cmake_checkversion_outputstr = cmake_checkversion_outputbytes.decode("utf-8")

        cmake_current_version_tuple = versionToTuple(cmake_checkversion_outputstr)
        cmake_target_version_tuple = versionToTuple(cmake_target_version)
        if cmake_current_version_tuple == cmake_target_version_tuple:
            dump(filename, "current cmake version is the same as the target {}".format(cmake_target_version))
        elif cmake_current_version_tuple > cmake_target_version_tuple:
            warn(filename, "current cmake version {} > target version {} (please check if with any issue in subsequent steps)".format(cmake_checkversion_outputstr, cmake_target_version))
        else:
            need_upgrade_cmake = True
            warn(filename, "current cmake version {} < target version {} (need to upgrade cmake)".format(cmake_checkversion_outputstr, cmake_target_version))

    if need_upgrade_cmake:
        prompt(filename, "check apt repo for CMake...")
        is_add_apt_repo_for_cmake = True
        cmake_repo_website = "https://apt.kitware.com/ubuntu"
        cmake_check_apt_repo_cmd = "sudo cat /etc/apt/sources.list | grep {}".format(cmake_repo_website)
        cmake_check_apt_repo_subprocess = subprocess.run(cmake_check_apt_repo_cmd, shell=True, capture_output=True)
        if cmake_check_apt_repo_subprocess.returncode != 0:
            die(filename, "failed to check apt repot for CMake")
        else:
            cmake_check_apt_repo_outputbytes = cmake_check_apt_repo_subprocess.stdout
            cmake_check_apt_repo_outputstr = cmake_check_apt_repo_outputbytes.decode("utf-8")

            if cmake_repo_website in cmake_check_apt_repo_outputstr:
                is_add_apt_repo_for_cmake = False

        if is_add_apt_repo_for_cmake:
            prompt(filename, "Add apt repo for CMake...")
            cmake_add_apt_repo_cmd = "wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add - && sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && sudo apt-get update && sudo apt-get install kitware-archive-keyring && sudo apt-key --keyring /etc/apt/trusted.gpg del C1F34CDD40CD72DA"
            cmake_add_apt_repo_subprocess = subprocess.run(cmake_add_apt_repo_cmd, shell=True)
            if cmake_add_apt_repo_subprocess.returncode != 0:
                die(filename, "failed to add apt repo for CMake")
        
        prompt(filename, "install CMake by apt...")
        cmake_install_cmd = "sudo apt-get install cmake"
        cmake_install_subprocess = subprocess.run(cmake_install_cmd)
        if cmake_install_subprocess.returncode != 0:
            die(filename, "failed to install CMake by apt")