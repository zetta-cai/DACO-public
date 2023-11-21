#!/usr/bin/env python3

import os

from utils.logutil import *

# Path-related variables and functions

system_bin_pathstr = os.getenv("PATH") # e.g., /usr/local/bin:/usr/local

def list_immediate_files_and_directories(directory):
    # Get a list of files and directories in the given directory
    items = os.listdir(directory)
    
    # Get immediate files and directories
    files_and_dirs = [item for item in items if (item != "." and item != ".." and (os.path.isfile(os.path.join(directory, item)) or os.path.isdir(os.path.join(directory, item))))]
    
    return files_and_dirs

# Return the first dirpath including target_name from env_pathstr
# If no dirpath includes target_name, the first dirpath including /usr or /usr/local will be returned
def getPreferredDirpathForTarget(scriptname, target_name, env_pathstr):
    if env_pathstr == None:
        die(scriptname, "env_pathstr for {} is None!",format(target_name))
    #else:
    #    dump(scriptname, "env_pathstr for {}: {}".format(target_name, env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in range(len(env_paths)):
        tmp_env_path = env_paths[i].strip()

        # Judge whether the file or directory target_name is under tmp_env_path, if tmp_env_path exists and is a directory
        if os.path.exists(tmp_env_path) and os.path.isdir(tmp_env_path):
            tmp_sub_files_or_dirs = list_immediate_files_and_directories(tmp_env_path)
            for tmp_sub_file_or_dir in tmp_sub_files_or_dirs:
                if target_name in tmp_sub_file_or_dir:
                    preferred_dirpath = tmp_env_path.strip()
                    break
    
    if preferred_dirpath == "":
        warn(scriptname, "{} are NOT found in {}!".format(target_name, env_pathstr))
        for i in range(len(env_paths)):
            tmp_env_path = env_paths[i].strip()

            if "/usr" in tmp_env_path or "/usr/local" in tmp_env_path:
                preferred_dirpath = tmp_env_path.strip()
                break
    
    if preferred_dirpath == "":
        die(scriptname, "/usr or /usr/local are also NOT found in {}!".format(env_pathstr))
    
    return preferred_dirpath