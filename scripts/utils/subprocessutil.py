#!/usr/bin/env python3

import os
import subprocess

from utils.logutil import *

old_alternative_priority = 10
new_alternative_priority = 100

# Subprocess-related variables and functions

## (1) Common functions

def versionToTuple(v):
    return tuple(map(int, (v.split("."))))

def runCmd(cmdstr, is_capture_output=True):
    print("[shell] {}".format(cmdstr))
    tmp_subprocess = subprocess.run(cmdstr, shell=True, capture_output=is_capture_output)
    return tmp_subprocess

def getSubprocessErrstr(tmp_subprocess):
    tmp_subprocess_errstr = ""
    if tmp_subprocess.stderr != None:
        tmp_subprocess_errbytes = tmp_subprocess.stderr
        tmp_subprocess_errstr = tmp_subprocess_errbytes.decode("utf-8")
    return tmp_subprocess_errstr

def getSubprocessOutputstr(tmp_subprocess):
    tmp_subprocess_outputstr = ""
    if tmp_subprocess.stdout != None:
        tmp_subprocess_outputbytes = tmp_subprocess.stdout
        tmp_subprocess_outputstr = tmp_subprocess_outputbytes.decode("utf-8")
    return tmp_subprocess_outputstr

## (2) For softwares from tarball or apt

def checkVersion(scriptname, software_name, target_version, checkversion_cmd):
    prompt(scriptname, "check version of {}...".format(software_name))
    checkversion_subprocess = runCmd(checkversion_cmd)
    need_upgrade = False
    if checkversion_subprocess.returncode != 0:
        die(scriptname, "failed to get the current version of {}".format(software_name))
    else:
        checkversion_outputstr = getSubprocessOutputstr(checkversion_subprocess)

        current_version_tuple = versionToTuple(checkversion_outputstr.split(" ")[-1])
        target_version_tuple = versionToTuple(target_version)
        if current_version_tuple == target_version_tuple:
            dump(scriptname, "current {} version is the same as the target {}".format(software_name, target_version))
        elif current_version_tuple > target_version_tuple:
            dump(scriptname, "current {} version {} > target version {} (please check if with any issue in subsequent steps)".format(software_name, checkversion_outputstr, target_version))
        else:
            need_upgrade = True
            warn(scriptname, "current {} version {} < target version {} (need to upgrade {})".format(software_name, checkversion_outputstr, target_version, software_name))
    return need_upgrade

def checkOldAlternative(scriptname, software_name):
    prompt(scriptname, "check if old {} is preserved...".format(software_name))
    check_old_alternative_cmd = "sudo update-alternatives --query {0} | grep $(readlink -f $(which {0}))".format(software_name)
    check_old_alternative_subprocess = runCmd(check_old_alternative_cmd)
    need_preserve_old_alternative = True;
    if check_old_alternative_subprocess.returncode == 0:
        check_old_alternative_outputstr = getSubprocessOutputstr(check_old_alternative_subprocess)
        if check_old_alternative_outputstr != "":
            need_preserve_old_alternative = False
    return need_preserve_old_alternative

def preserveOldAlternative(scriptname, software_name, preferred_binpath):
    prompt(scriptname, "preserve old {}...".format(software_name))
    preserve_old_cmd = "sudo update-alternatives --install {0}/{1} {1} $(readlink -f $(which {1}) {2}".format(preferred_binpath, software_name, old_alternative_priority)
    preserve_old_subprocess = runCmd(preserve_old_cmd)
    if preserve_old_subprocess.returncode != 0:
        die(scriptname, "failed to preserve old {}".format(software_name))

def downloadTarball(scriptname, download_filepath, download_url):
    tmp_parent_dirpath = os.path.dirname(download_filepath)
    tmp_download_filename = os.path.basename(download_filepath)
    if not os.path.exists(download_filepath):
        prompt(scriptname, "download {}...".format(download_filepath))
        download_cmd = "cd {} && wget {}".format(tmp_parent_dirpath, download_url)

        download_subprocess = runCmd(download_cmd)
        if download_subprocess.returncode != 0:
            die(scriptname, "failed to download {}".format(download_filepath))
    else:
        dump(scriptname, "{} exists ({} has been downloaded)".format(download_filepath, tmp_download_filename))

def decompressTarball(scriptname, download_filepath, decompress_dirpath, decompress_tool):
    tmp_parent_dirpath = os.path.dirname(download_filepath)
    tmp_download_filename = os.path.basename(download_filepath)
    if not os.path.exists(decompress_dirpath):
        prompt(scriptname, "decompress {}...".format(download_filepath))
        decompress_cmd = "cd {} && {} {}".format(tmp_parent_dirpath, decompress_tool, tmp_download_filename)

        decompress_subprocess = runCmd(decompress_cmd)
        if decompress_subprocess.returncode != 0:
            die(scriptname, "failed to decompress {}".format(download_filepath))
    else:
        dump(scriptname, "{} exists ({} has been decompressed)".format(decompress_dirpath, tmp_download_filename))

def installDecompressedTarball(scriptname, decompress_dirpath, install_filepath, install_tool, pre_install_tool = None, time_consuming = False):
    tmp_parent_dirpath = os.path.dirname(decompress_dirpath)
    tmp_decompress_dirname = os.path.basename(decompress_dirpath)
    if not os.path.exists(install_filepath):
        if time_consuming == False:
            prompt(scriptname, "install {} from source...".format(tmp_decompress_dirname))
        else:
            prompt(scriptname, "install {} from source (it takes some time)...".format(tmp_decompress_dirname))
        install_cmd = "cd {} && {}".format(decompress_dirpath, install_tool)
        if pre_install_tool != None:
            install_cmd = "{} && {}".format(pre_install_tool, install_cmd)
        install_subprocess = runCmd(install_cmd)
        if install_subprocess.returncode != 0:
            die(scriptname, "failed to install {}".format(tmp_decompress_dirname))
    else:
        dump(scriptname, "{} exists ({} has been installed)".format(install_filepath, tmp_decompress_dirname))

def installByApt(scriptname, software_name, apt_targetname):
    prompt(scriptname, "install {} by apt...".format(software_name))
    install_cmd = "sudo apt install {}".format(apt_targetname)
    install_subprocess = runCmd(install_cmd)
    if install_subprocess.returncode != 0:
        die(scriptname, "failed to install {}".format(apt_targetname))

def preserveNewAlternative(scriptname, software_name, preferred_binpath, install_filepath):
    tmp_parent_dirpath = os.path.dirname(install_filepath)
    tmp_install_filename = os.path.basename(install_filepath)

    prompt(scriptname, "switch to {}...".format(tmp_install_filename))
    switch_new_cmd = "sudo update-alternatives --install {0}/{1} {1} {2} {3}".format(preferred_binpath, software_name, install_filepath, new_alternative_priority)
    switch_new_subprocess = runCmd(switch_new_cmd)
    if switch_new_subprocess.returncode != 0:
        die(scriptname, "failed to switch {}".format(install_filepath))

def clearTarball(scriptname, download_filepath):
    tmp_parent_dirpath = os.path.dirname(download_filepath)
    tmp_download_filename = os.path.basename(download_filepath)

    prompt(scriptname, "clear {}".format(download_filepath))
    clear_cmd = "cd {} && rm {}".format(tmp_parent_dirpath, tmp_download_filename)

    clear_subprocess = runCmd(clear_cmd)
    if clear_subprocess.returncode != 0:
        die(scriptname, "failed to clear {}".format(download_filepath))

## (3) For softwares from github

def cloneRepo(scriptname, clone_dirpath, software_name, repo_url):
    if not os.path.exists(clone_dirpath):
        tmp_parent_dirpath = os.path.dirname(clone_dirpath)
        tmp_repo_dirname = os.path.basename(clone_dirpath)

        prompt(scriptname, "clone {} into {}...".format(software_name, clone_dirpath))
        clone_cmd = "cd {} && git clone {} {}".format(tmp_parent_dirpath, repo_url, tmp_repo_dirname)

        clone_subprocess = runCmd(clone_cmd)
        if clone_subprocess.returncode != 0:
            clone_errstr = getSubprocessErrstr(clone_subprocess)
            die(scriptname, "failed to clone {} into {}; error: {}".format(software_name, clone_dirpath, clone_errstr))
    else:
        dump(scriptname, "{} exists ({} has been cloned)".format(clone_dirpath, software_name))

def checkoutCommit(scriptname, clone_dirpath, software_name, target_commit):
    check_commit_cmd = "cd {} && git log --format=\"%H\" -n 1".format(clone_dirpath)
    check_commit_subprocess = runCmd(check_commit_cmd)
    if check_commit_subprocess.returncode != 0:
        check_commit_errstr = getSubprocessErrstr(check_commit_subprocess)
        die(scriptname, "failed to get the latest commit ID of {}; error: {}".format(software_name, check_commit_errstr))
    else:
        check_commit_outputstr = getSubprocessOutputstr(check_commit_subprocess)
        if target_commit not in check_commit_outputstr:
            prompt(scriptname, "the latest commit ID of {0} is {1} -> reset {0} to commit {2}...".format(software_name, check_commit_outputstr, target_commit))
            reset_cmd = "cd {} && git reset --hard {}".format(clone_dirpath, target_commit)
            
            reset_subprocess = runCmd(reset_cmd)
            if reset_subprocess.returncode != 0:
                reset_errstr = getSubprocessErrstr(reset_subprocess)
                die(scriptname, "failed to reset {}; error: {}".format(software_name, reset_errstr))
        else:
            dump(scriptname, "the latest commit ID of {} is already {}".format(software_name, target_commit))

def installFromRepoIfNot(scriptname, install_dirpath, software_name, clone_dirpath, install_tool, time_consuming = False):
    if not os.path.exists(install_dirpath):
        installFromRepo(scriptname, software_name, clone_dirpath, install_tool, time_consuming)
    else:
        dump(scriptname, "{} exists ({} has been installed)".format(install_dirpath, software_name))

def installFromRepo(scriptname, software_name, clone_dirpath, install_tool, time_consuming = False):
    if time_consuming == False:
        prompt(scriptname, "install {}...".format(software_name))
    else:
        prompt(scriptname, "install {} (it takes some time)...".format(software_name))
    install_cmd = "cd {} && {}".format(clone_dirpath, install_tool)

    if time_consuming == False:
        install_subprocess = runCmd(install_cmd)
    else:
        install_subprocess = runCmd(install_cmd, is_capture_output=False)
    if install_subprocess.returncode != 0:
        install_errstr = getSubprocessErrstr(install_subprocess)
        die(scriptname, "failed to install {}; error: {}".format(software_name, install_errstr))