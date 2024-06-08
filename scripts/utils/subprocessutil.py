#!/usr/bin/env python3
# SubprocessUtil: subprocess utilities for common shell operations

import os
import subprocess

from .logutil import *

class SubprocessUtil:

    old_alternative_priority_ = 10
    new_alternative_priority_ = 100

    # Subprocess-related variables and functions

    ## (1) Common functions

    @staticmethod
    def versionToTuple_(v):
        return tuple(map(int, (v.split("."))))

    @staticmethod
    def runCmd(cmdstr, is_capture_output=True, keep_silent=False):
        if not keep_silent:
            print("[shell] {}".format(cmdstr), flush = True)
        # Deprecated due to only supported by python >= 3.7
        #tmp_subprocess = subprocess.run(cmdstr, shell=True, capture_output=is_capture_output)
        if not is_capture_output:
            tmp_subprocess = subprocess.run(cmdstr, shell=True)
        else:
            tmp_subprocess = subprocess.run(cmdstr, shell=True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        return tmp_subprocess

    @staticmethod
    def getSubprocessErrstr(tmp_subprocess):
        tmp_subprocess_errstr = ""
        if tmp_subprocess.stderr != None:
            tmp_subprocess_errbytes = tmp_subprocess.stderr
            tmp_subprocess_errstr = tmp_subprocess_errbytes.decode("utf-8")
        return tmp_subprocess_errstr

    @staticmethod
    def getSubprocessOutputstr(tmp_subprocess):
        tmp_subprocess_outputstr = ""
        if tmp_subprocess.stdout != None:
            tmp_subprocess_outputbytes = tmp_subprocess.stdout
            tmp_subprocess_outputstr = tmp_subprocess_outputbytes.decode("utf-8")
        return tmp_subprocess_outputstr

    @staticmethod
    def tryToCreateDirectory(scriptname, dirpath, keep_silent=False):
        if not os.path.exists(dirpath):
            if not keep_silent:
                LogUtil.prompt(scriptname, "create directory {}...".format(dirpath))
            tmp_mkdirs_cmd = "mkdir -p {}".format(dirpath)
            tmp_mkdirs_subprocess = SubprocessUtil.runCmd(tmp_mkdirs_cmd)
            if tmp_mkdirs_subprocess.returncode != 0:
                LogUtil.die(Common.scriptname, "failed to create directory {} (errmsg: {})".format(dirpath, SubprocessUtil.getSubprocessErrstr(tmp_mkdirs_subprocess)))
    
    @staticmethod
    def moveSrcToTarget(scriptname, srcpath, targetpath, keep_silent=False):
        if not os.path.exists(srcpath):
            if not keep_silent:
                LogUtil.prompt(scriptname, "move {} to {}...".format(srcpath, targetpath))
            tmp_move_cmd = "mv {} {}".format(srcpath, targetpath)
            tmp_move_subprocess = SubprocessUtil.runCmd(tmp_move_cmd)
            if tmp_move_subprocess.returncode != 0:
                LogUtil.die(Common.scriptname, "failed to move {} to {} (errmsg: {})".format(srcpath, targetpath, SubprocessUtil.getSubprocessErrstr(tmp_move_subprocess)))
        else:
            LogUtil.die(Common.scriptname, "The source path {} does not exist".format(srcpath))

    ## (2) For softwares from tarball or apt

    @classmethod
    def checkVersion(cls, scriptname, software_name, target_version, checkversion_cmd):
        LogUtil.prompt(scriptname, "check version of {}...".format(software_name))
        checkversion_subprocess = cls.runCmd(checkversion_cmd)
        need_upgrade = False
        current_version = ""
        if checkversion_subprocess.returncode != 0:
            LogUtil.warn(scriptname, "failed to get the current version of {} (errmsg: {})".format(software_name, cls.getSubprocessErrstr(checkversion_subprocess)))
            need_upgrade = True
            current_version = None
        else:
            checkversion_outputstr = cls.getSubprocessOutputstr(checkversion_subprocess)
            # Filter out unnecessary elements
            checkversion_firstline_elements = []
            for tmp_element in checkversion_outputstr.splitlines()[0].split(" "):
                if "." in tmp_element: # Contain dots
                    checkversion_firstline_elements.append(tmp_element)
            current_version = checkversion_firstline_elements[-1]
            current_version_tuple = cls.versionToTuple_(current_version)
            target_version_tuple = cls.versionToTuple_(target_version)
            if current_version_tuple == target_version_tuple:
                LogUtil.dump(scriptname, "current {} version is the same as the target {}".format(software_name, target_version))
            elif current_version_tuple > target_version_tuple:
                LogUtil.dump(scriptname, "current {} version {} > target version {} (please check if with any issue in subsequent steps)".format(software_name, current_version, target_version))
            else:
                need_upgrade = True
                LogUtil.warn(scriptname, "current {} version {} < target version {} (need to upgrade {})".format(software_name, current_version, target_version, software_name))
        return need_upgrade, current_version

    @classmethod
    def getCanonicalFilepath(cls, scriptname, software_name, current_version):
        # Get software filepath, e.g., /usr/bin/gcc
        software_filepath = ""
        get_software_filepath_cmd = "which {}".format(software_name)
        get_software_filepath_subprocess = cls.runCmd(get_software_filepath_cmd)
        if get_software_filepath_subprocess.returncode != 0:
            LogUtil.die(scriptname, cls.getSubprocessErrstr(get_software_filepath_subprocess))
        else:
            software_filepath = cls.getSubprocessOutputstr(get_software_filepath_subprocess).strip()

        # Get canonical filepath, e.g., /usr/bin/x86_64-linux-gnu-gcc-7
        canonical_filepath = ""
        get_canonical_filepath_cmd = "readlink -f $(which {})".format(software_name)
        get_canonical_filepath_subprocess = cls.runCmd(get_canonical_filepath_cmd)
        if get_canonical_filepath_subprocess.returncode != 0:
            LogUtil.die(scriptname, cls.getSubprocessErrstr(get_canonical_filepath_subprocess))
        else:
            canonical_filepath = cls.getSubprocessOutputstr(get_canonical_filepath_subprocess).strip()
        
        new_canonical_filepath = ""
        if canonical_filepath == software_filepath: # Need to copy software to a canonical filepath for preservation later
            new_canonical_filepath = os.path.join(os.path.dirname(canonical_filepath), "{}-{}".format(software_name, current_version)) # E.g., /usr/bin/gcc-9.4.0
            copy_software_cmd = "sudo cp {} {}".format(software_filepath, new_canonical_filepath)
            copy_software_subprocess = cls.runCmd(copy_software_cmd)
            if copy_software_subprocess.returncode != 0:
                LogUtil.die(scriptname, cls.getSubprocessErrstr(copy_software_subprocess))
        else:
            new_canonical_filepath = canonical_filepath
        
        return new_canonical_filepath

    @classmethod
    def checkAlternative(cls, scriptname, software_name, canonical_filepath):
        LogUtil.prompt(scriptname, "check if alternative {} is already preserved...".format(software_name))
        check_alternative_cmd = "sudo update-alternatives --query {} | grep {}".format(software_name, canonical_filepath)
        check_alternative_subprocess = cls.runCmd(check_alternative_cmd)
        need_preserve_alternative = True;
        if check_alternative_subprocess.returncode == 0:
            check_alternative_outputstr = cls.getSubprocessOutputstr(check_alternative_subprocess)
            if check_alternative_outputstr != "":
                need_preserve_alternative = False
        return need_preserve_alternative

    @classmethod
    def preserveOldAlternative(cls, scriptname, software_name, preferred_binpath, canonical_filepath):
        LogUtil.prompt(scriptname, "preserve old {}...".format(software_name))
        preserve_old_cmd = "sudo update-alternatives --install {0}/{1} {1} {2} {3}".format(preferred_binpath, software_name, canonical_filepath, cls.old_alternative_priority_)
        preserve_old_subprocess = cls.runCmd(preserve_old_cmd)
        if preserve_old_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to preserve old {} (errmsg: {})".format(software_name, cls.getSubprocessErrstr(preserve_old_subprocess)))

    @classmethod
    def downloadTarball(cls, scriptname, download_filepath, download_url):
        tmp_parent_dirpath = os.path.dirname(download_filepath)
        SubprocessUtil.tryToCreateDirectory(scriptname, tmp_parent_dirpath)

        tmp_download_filename = os.path.basename(download_filepath)
        if not os.path.exists(download_filepath):
            LogUtil.prompt(scriptname, "download {}...".format(download_filepath))
            download_cmd = "cd {} && wget {}".format(tmp_parent_dirpath, download_url)

            download_subprocess = cls.runCmd(download_cmd)
            if download_subprocess.returncode != 0:
                LogUtil.die(scriptname, "failed to download {} (errmsg: {})".format(download_filepath, cls.getSubprocessErrstr(download_subprocess)))
        else:
            LogUtil.dump(scriptname, "{} exists ({} has been downloaded)".format(download_filepath, tmp_download_filename))

    @classmethod
    def decompressTarball(cls, scriptname, download_filepath, decompress_dirpath, decompress_tool):
        tmp_parent_dirpath = os.path.dirname(download_filepath)
        tmp_download_filename = os.path.basename(download_filepath)
        if not os.path.exists(decompress_dirpath):
            LogUtil.prompt(scriptname, "decompress {}...".format(download_filepath))
            decompress_cmd = "cd {} && {} {}".format(tmp_parent_dirpath, decompress_tool, tmp_download_filename)

            decompress_subprocess = cls.runCmd(decompress_cmd)
            if decompress_subprocess.returncode != 0:
                LogUtil.die(scriptname, "failed to decompress {} (errmsg: {})".format(download_filepath, cls.getSubprocessErrstr(decompress_subprocess)))
        else:
            LogUtil.dump(scriptname, "{} exists ({} has been decompressed)".format(decompress_dirpath, tmp_download_filename))

    @classmethod
    def installDecompressedTarball(cls, scriptname, decompress_dirpath, install_filepath, install_tool, pre_install_tool = None, time_consuming = False):
        tmp_parent_dirpath = os.path.dirname(decompress_dirpath)
        tmp_decompress_dirname = os.path.basename(decompress_dirpath)
        if not os.path.exists(install_filepath):
            if time_consuming == False:
                LogUtil.prompt(scriptname, "install {} from source...".format(tmp_decompress_dirname))
            else:
                LogUtil.prompt(scriptname, "install {} from source (it takes some time)...".format(tmp_decompress_dirname))

            install_cmd = "cd {} && {}".format(decompress_dirpath, install_tool)
            if pre_install_tool != None:
                install_cmd = "{} && {}".format(pre_install_tool, install_cmd)
            if time_consuming == False:
                install_subprocess = cls.runCmd(install_cmd)
                if install_subprocess.returncode != 0:
                    install_errstr = cls.getSubprocessErrstr(install_subprocess)
                    LogUtil.die(scriptname, "failed to install {} (errmsg: {})".format(tmp_decompress_dirname, install_errstr))
            else:
                install_subprocess = cls.runCmd(install_cmd, is_capture_output=False)
                if install_subprocess.returncode != 0:
                    LogUtil.die(scriptname, "failed to install {} (errmsg: {})".format(tmp_decompress_dirname, cls.getSubprocessErrstr(install_subprocess)))
        else:
            LogUtil.dump(scriptname, "{} exists ({} has been installed)".format(install_filepath, tmp_decompress_dirname))

    @classmethod
    def installByApt(cls, scriptname, software_name, apt_targetname, target_version=None):
        apt_target_fullname = ""
        if target_version is not None:
            # Get full aptlib name of the software with the target veriosn
            get_apt_target_fullname_cmd = "apt-cache policy {} | grep {}".format(apt_targetname, target_version)
            get_apt_target_fullname_subprocess = cls.runCmd(get_apt_target_fullname_cmd)
            if get_apt_target_fullname_subprocess.returncode != 0:
                if cls.getSubprocessErrstr(get_apt_target_fullname_subprocess) != "":
                    LogUtil.die(scriptname, "failed to get the full name of {} (errmsg: {})".format(apt_targetname, cls.getSubprocessErrstr(get_apt_target_fullname_subprocess)))
                else:
                    LogUtil.die(scriptname, "NOT found {} {} in apt".format(apt_targetname, target_version))
            else:
                apt_target_fullname = "{}={}".format(apt_targetname, cls.getSubprocessOutputstr(get_apt_target_fullname_subprocess).strip().split(" ")[0])
        else:
            apt_target_fullname = apt_targetname

        LogUtil.prompt(scriptname, "install {} {} by apt...".format(software_name, target_version))
        install_cmd = "sudo apt install -y {}".format(apt_target_fullname)
        install_subprocess = cls.runCmd(install_cmd, is_capture_output=False)
        if install_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to install {} (errmsg: {})".format(apt_target_fullname, cls.getSubprocessErrstr(install_subprocess)))
    
    @classmethod
    def upgradeByApt(cls, scriptname, software_name, apt_targetname):
        LogUtil.prompt(scriptname, "upgrade {} by apt...".format(software_name))
        upgrade_cmd = "sudo apt-get upgrade -y {}".format(apt_targetname)
        upgrade_subprocess = cls.runCmd(upgrade_cmd, is_capture_output=False)
        if upgrade_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to upgrade {} (errmsg: {})".format(apt_target_fullname, cls.getSubprocessErrstr(upgrade_subprocess)))

    @classmethod
    def preserveNewAlternative(cls, scriptname, software_name, preferred_binpath, install_filepath):
        tmp_parent_dirpath = os.path.dirname(install_filepath)
        tmp_install_filename = os.path.basename(install_filepath)

        LogUtil.prompt(scriptname, "switch to {}...".format(tmp_install_filename))
        switch_new_cmd = "sudo update-alternatives --install {0}/{1} {1} {2} {3}".format(preferred_binpath, software_name, install_filepath, cls.new_alternative_priority_)
        switch_new_subprocess = cls.runCmd(switch_new_cmd)
        if switch_new_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to switch {} (errmsg: {})".format(install_filepath, cls.getSubprocessErrstr(switch_new_subprocess)))

    @classmethod
    def clearTarball(cls, scriptname, download_filepath):
        tmp_parent_dirpath = os.path.dirname(download_filepath)
        tmp_download_filename = os.path.basename(download_filepath)

        LogUtil.prompt(scriptname, "clear {}".format(download_filepath))
        clear_cmd = "cd {} && rm {}".format(tmp_parent_dirpath, tmp_download_filename)

        clear_subprocess = cls.runCmd(clear_cmd)
        if clear_subprocess.returncode != 0:
            LogUtil.die(scriptname, "failed to clear {} (errmsg: {})".format(download_filepath, cls.getSubprocessErrstr(clear_subprocess)))

    ## (3) For softwares from github

    @classmethod
    def cloneRepo(cls, scriptname, clone_dirpath, software_name, repo_url, with_subrepo = False):
        if not os.path.exists(clone_dirpath):
            tmp_parent_dirpath = os.path.dirname(clone_dirpath)
            tmp_repo_dirname = os.path.basename(clone_dirpath)

            LogUtil.prompt(scriptname, "clone {} into {}...".format(software_name, clone_dirpath))
            clone_cmd = ""
            if with_subrepo:
                clone_cmd = "cd {} && git clone --recursive {} {}".format(tmp_parent_dirpath, repo_url, tmp_repo_dirname)
            else:
                clone_cmd = "cd {} && git clone {} {}".format(tmp_parent_dirpath, repo_url, tmp_repo_dirname)

            clone_subprocess = cls.runCmd(clone_cmd)
            if clone_subprocess.returncode != 0:
                clone_errstr = cls.getSubprocessErrstr(clone_subprocess)
                LogUtil.die(scriptname, "failed to clone {} into {} (errmsg: {})".format(software_name, clone_dirpath, clone_errstr))
        else:
            LogUtil.dump(scriptname, "{} exists ({} has been cloned)".format(clone_dirpath, software_name))

    @classmethod
    def checkoutCommit(cls, scriptname, clone_dirpath, software_name, target_commit):
        check_commit_cmd = "cd {} && git log --format=\"%H\" -n 1".format(clone_dirpath)
        check_commit_subprocess = cls. runCmd(check_commit_cmd)
        if check_commit_subprocess.returncode != 0:
            check_commit_errstr = cls.getSubprocessErrstr(check_commit_subprocess)
            LogUtil.die(scriptname, "failed to get the latest commit ID of {} (errmsg: {})".format(software_name, check_commit_errstr))
        else:
            check_commit_outputstr = cls.getSubprocessOutputstr(check_commit_subprocess)
            if target_commit not in check_commit_outputstr:
                LogUtil.prompt(scriptname, "the latest commit ID of {0} is {1} -> reset {0} to commit {2}...".format(software_name, check_commit_outputstr, target_commit))
                reset_cmd = "cd {} && git reset --hard {}".format(clone_dirpath, target_commit)
                
                reset_subprocess = cls.runCmd(reset_cmd)
                if reset_subprocess.returncode != 0:
                    reset_errstr = cls.getSubprocessErrstr(reset_subprocess)
                    LogUtil.die(scriptname, "failed to reset {} (errmsg: {})".format(software_name, reset_errstr))
            else:
                LogUtil.dump(scriptname, "the latest commit ID of {} is already {}".format(software_name, target_commit))

    @classmethod
    def installFromRepoIfNot(cls, scriptname, install_dirpath, software_name, clone_dirpath, install_tool, pre_install_tool = None, time_consuming = False):
        if not os.path.exists(install_dirpath):
            cls.installFromRepo(scriptname, software_name, clone_dirpath, install_tool, pre_install_tool, time_consuming)
        else:
            LogUtil.dump(scriptname, "{} exists ({} has been installed)".format(install_dirpath, software_name))

    @classmethod
    def installFromRepo(cls, scriptname, software_name, clone_dirpath, install_tool, pre_install_tool = None, time_consuming = False):
        if time_consuming == False:
            LogUtil.prompt(scriptname, "install {}...".format(software_name))
        else:
            LogUtil.prompt(scriptname, "install {} (it takes some time)...".format(software_name))

        install_cmd = "cd {} && {}".format(clone_dirpath, install_tool)
        if pre_install_tool != None:
            install_cmd = "{} && {}".format(pre_install_tool, install_cmd)

        if time_consuming == False:
            install_subprocess = cls.runCmd(install_cmd)
            if install_subprocess.returncode != 0:
                LogUtil.die(scriptname, "failed to install {} (errmsg: {})".format(software_name, cls.getSubprocessErrstr(install_subprocess)))
        else:
            install_subprocess = cls.runCmd(install_cmd, is_capture_output=False)
            if install_subprocess.returncode != 0:
                install_errstr = cls.getSubprocessErrstr(install_subprocess)
                LogUtil.die(scriptname, "failed to install {} (errmsg: {})".format(software_name, install_errstr))