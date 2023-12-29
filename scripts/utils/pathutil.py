#!/usr/bin/env python3
# PathUtil: path-related utilities to operate files/folders

import os

from .logutil import *
from .subprocessutil import *

class PathUtil:

    # Path-related variables and functions

    system_env_pathstr_ = os.getenv("PATH") # e.g., /usr/local/bin:/usr/local

    @staticmethod
    def list_immediate_files_and_directories(directory):
        # Get a list of files and directories in the given directory
        items = os.listdir(directory)
        
        # Get immediate files and directories
        files_and_dirs = [item for item in items if (item != "." and item != ".." and (os.path.isfile(os.path.join(directory, item)) or os.path.isdir(os.path.join(directory, item))))]
        
        return files_and_dirs

    # Return the first dirpath including target_name from system_env_pathstr_
    # If no dirpath includes target_name, the first dirpath including /usr or /usr/local will be returned
    @classmethod
    def getPreferredDirpathForTarget(cls, scriptname, target_name):
        if cls.system_env_pathstr_ == None:
            LogUtil.die(scriptname, "system_env_pathstr for {} is None!",format(target_name))
        #else:
        #    LogUtil.dump(scriptname, "system_env_pathstr for {}: {}".format(target_name, cls.system_env_pathstr_))

        preferred_dirpath = ""
        env_paths = cls.system_env_pathstr_.split(":")
        for i in range(len(env_paths)):
            tmp_env_path = env_paths[i].strip()

            # Judge whether the file or directory target_name is under tmp_env_path, if tmp_env_path exists and is a directory
            if os.path.exists(tmp_env_path) and os.path.isdir(tmp_env_path):
                tmp_sub_files_or_dirs = cls.list_immediate_files_and_directories(tmp_env_path)
                for tmp_sub_file_or_dir in tmp_sub_files_or_dirs:
                    if target_name in tmp_sub_file_or_dir:
                        preferred_dirpath = tmp_env_path.strip()
                        break
            
            if preferred_dirpath != "":
                break
        
        if preferred_dirpath == "":
            LogUtil.warn(scriptname, "{} are NOT found in {}!".format(target_name, cls.system_env_pathstr_))
            for i in range(len(env_paths)):
                tmp_env_path = env_paths[i].strip()

                if "/usr" in tmp_env_path or "/usr/local" in tmp_env_path:
                    preferred_dirpath = tmp_env_path.strip()
                    break
        
        if preferred_dirpath == "":
            LogUtil.die(scriptname, "/usr or /usr/local are also NOT found in {}!".format(cls.system_env_pathstr_))
        
        return preferred_dirpath

    @staticmethod
    def replace_dir(scriptname, original_dir, current_dir, path):
        if not os.path.exists(path):
            LogUtil.warn(scriptname, "Path does not exist: " + path)
            return
        
        replace_rootdir_cmd = "sed -i 's!{}!{}!g' {}".format(original_dir, current_dir, path)
        replace_rootdir_subprocess = SubprocessUtil.runCmd(replace_rootdir_cmd)
        if replace_rootdir_subprocess.returncode != 0:
            LogUtil.die(scriptname, "Failed to replace rootdir: {} (errmsg: {})".format(path, SubprocessUtil.getSubprocessErrstr(replace_rootdir_subprocess)))

    @staticmethod
    def restore_dir(scriptname, original_dir, current_dir, path):
        if not os.path.exists(path):
            LogUtil.warn(scriptname, "Path does not exist: " + path)
            return
        
        restore_rootdir_cmd = "sed -i 's!{}!{}!g' {}".format(current_dir, original_dir, path)
        restore_rootdir_subprocess = SubprocessUtil.runCmd(restore_rootdir_cmd)
        if restore_rootdir_subprocess.returncode != 0:
            LogUtil.die(scriptname, "Failed to restore rootdir: {} (errmsg: {})".format(path, SubprocessUtil.getSubprocessErrstr(restore_rootdir_subprocess)))