#!/usr/bin/env python3

import os
import sys
import subprocess

# TMPDEBUG
only_cachelib = True

# (1) Path-related variables and functions

filename = sys.argv[0]
proj_dirname = os.path.dirname(os.path.dirname(os.path.abspath(filename)))

#username = os.getenv("USER") # NOTE: USER will be root if sudo is used
username = os.getenv("SUDO_USER")

# TODO: update lib_dirpath for new library installation path if necessary
lib_dirpath = "{}/lib".format(proj_dirname)
if not os.path.exists(lib_dirpath):
    print("{}: Create directory {}...".format(filename, lib_dirpath))
    os.mkdir(lib_dirpath)
#else:
#    print("{}: {} exists (directory has been created)".format(filename, lib_dirpath))

def list_immediate_files_and_directories(directory):
    # Get a list of files and directories in the given directory
    items = os.listdir(directory)
    
    # Get immediate files and directories
    files_and_dirs = [item for item in items if (item != "." and item != ".." and (os.path.isfile(os.path.join(directory, item)) or os.path.isdir(os.path.join(directory, item))))]
    
    return files_and_dirs

# Return the first dirpath including target_name from env_pathstr
# If no dirpath includes target_name, the first dirpath including /usr or /usr/local will be returned
def getPreferredDirpathForTarget(target_name, env_pathstr):
    if env_pathstr == None:
        print("ERROR: env_pathstr for {} is None!",format(target_name))
        sys.exit(1)
    #else:
    #    print("env_pathstr for {}: {}".format(target_name, env_pathstr))

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
        print("WARB: {} are NOT found in {}!".format(target_name, env_pathstr))
        for i in range(len(env_paths)):
            tmp_env_path = env_paths[i].strip()

            if "/usr" in tmp_env_path or "/usr/local" in tmp_env_path:
                preferred_dirpath = tmp_env_path.strip()
                break
    
    if preferred_dirpath == "":
        print("ERROR: /usr or /usr/local are also NOT found in {}!".format(env_pathstr))
        sys.exit(1)
    
    return preferred_dirpath

system_bin_pathstr = os.getenv("PATH") # e.g., /usr/local/bin:/usr/local

# For python3
python3_preferred_binpath = getPreferredDirpathForTarget("python3", system_bin_pathstr) # e.g., /usr/local/bin
python3_installpath = os.path.dirname(python3_preferred_binpath) # e.g., /usr/local
python3_install_binpath = "{}/bin".format(python3_installpath) # e.g., /usr/local/bin

# For gcc/g++
compiler_preferred_binpaths = {}
for compiler_name in ["gcc", "g++"]:
    compiler_preferred_binpaths[compiler_name] = getPreferredDirpathForTarget(compiler_name, system_bin_pathstr) # e.g., /usr/local/bin
compiler_installpath = "/usr" # Installed by apt
compiler_install_binpath = "{}/bin".format(compiler_installpath) # /usr/bin

#print("{} {}".format(python3_preferred_binpath, compiler_preferred_binpaths))

# For cmake
#cmake_installpath = "/usr" # Installed by apt
#cmake_install_binpath = "{}/bin".format(cmake_installpath)

# (2) Subprocess-related variables and functions

def runCmd(cmdstr):
    print("[shell] {}".format(cmdstr))
    tmp_subprocess = subprocess.run(cmdstr, shell=True, capture_output=True)
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