#!/usr/bin/env python3

import os
import sys
import subprocess

filename = sys.argv[0]
proj_dirname = os.path.dirname(os.path.dirname(os.path.abspath(filename)))

username = os.getenv("USER")

lib_dirpath = "lib"
if not os.path.exists(lib_dirpath):
    print("{}: Create directory {}...".format(filename, lib_dirpath))
    os.mkdir(lib_dirpath)
else:
    print("{}: {} exists (directory has been created)".format(filename, lib_dirpath))

def list_immediate_files_and_directories(directory):
    # Get a list of files and directories in the given directory
    items = os.listdir(directory)
    
    # Get immediate files and directories
    files_and_dirs = [item for item in items if (item != "." and item != ".." and (os.path.isfile(os.path.join(directory, item)) or os.path.isdir(os.path.join(directory, item))))]
    
    return files_and_dirs

# Return the first dirpath including target_name from env_pathstr
# If no dirpath includes target_name, the first dirpath including /usr or /usr/local will be returned
def getPreferredDirpathForTarget(infix_name, target_name, env_pathstr):
    if env_pathstr == None:
        print("ERROR: env_pathstr for {} is None!",format(target_name))
        sys.exit(1)
    else:
        print("env_pathstr for {}: {}".format(target_name, env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in range(len(env_paths)):
        tmp_env_path = env_paths[i].strip()

        # Judge whether the file or directory target_name is under tmp_env_path, if tmp_env_path exists and is a directory
        if "/usr/{}".format(infix_name) in tmp_env_path or "/usr/local/{}".format(infix_name) in tmp_env_path:
            tmp_sub_files_or_dirs = list_immediate_files_and_directories(tmp_env_path)
            for tmp_sub_file_or_dir in tmp_sub_files_or_dirs:
                if target_name in tmp_sub_file_or_dir:
                    preferred_dirpath = tmp_env_path.strip()
                    break
    
    if preferred_dirpath == "":
        print("ERROR: /usr/{0}/{1} or /usr/local/{0}/{1} are also NOT found in {2}!".format(infix_name, target_name, env_pathstr))
        sys.exit(1)
    
    return preferred_dirpath

# def getDefaultIncludeAndLibPathstr():
#     get_gpp_include_pathstr_cmd = "echo | g++ -E -Wp,-v -x c++ - 2>&1 | grep -v '#' | grep -v '^$' | grep -v 'search starts here:' | grep -v 'End of search list.' | grep -v 'ignoring'"
#     get_gpp_include_pathstr_subprocess = subprocess.run(get_gpp_include_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_include_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_include_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_include_pathstr_outputbytes = get_gpp_include_pathstr_subprocess.stdout
#         get_gpp_include_pathstr_outputstr = get_gpp_include_pathstr_outputbytes.decode("utf-8")

#         gpp_include_pathstr_elements = get_gpp_include_pathstr_outputstr.split("\n")
#         for i in range(len(gpp_include_pathstr_elements)):
#             gpp_include_pathstr_elements[i] = gpp_include_pathstr_elements[i].strip()
#         gpp_include_pathstr = ":".join(gpp_include_pathstr_elements)
    
#     get_gpp_lib_pathstr_cmd = "echo | g++ -E -v -x c++ - 2>&1 | grep 'LIBRARY_PATH'"
#     get_gpp_lib_pathstr_subprocess = subprocess.run(get_gpp_lib_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_lib_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_lib_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_lib_pathstr_outputbytes = get_gpp_lib_pathstr_subprocess.stdout
#         get_gpp_lib_pathstr_outputstr = get_gpp_lib_pathstr_outputbytes.decode("utf-8")

#         gpp_lib_pathstr = get_gpp_lib_pathstr_outputstr.split("=")[1]

#     return gpp_include_pathstr, gpp_lib_pathstr


system_bin_pathstr = os.getenv("PATH") # e.g., /usr/local/bin:/usr/local

# For python3
python3_preferred_binpath = getPreferredDirpathForTarget("bin", "python3", system_bin_pathstr) # e.g., /usr/local/bin
python3_installpath = os.path.dirname(python3_preferred_binpath) # e.g., /usr/local
python3_install_binpath = "{}/bin".format(python3_installpath) # e.g., /usr/local/bin

# For gcc/g++
compiler_preferred_binpaths = {}
for compiler_name in ["gcc", "g++"]:
    compiler_preferred_binpaths[compiler_name] = getPreferredDirpathForTarget("bin", compiler_name, system_bin_pathstr) # e.g., /usr/local/bin
compiler_installpath = "/usr" # Installed by apt
compiler_install_binpath = "{}/bin".format(compiler_installpath) # /usr/bin

#print("{} {}".format(python3_preferred_binpath, compiler_preferred_binpaths))

# For cmake
#cmake_installpath = "/usr" # Installed by apt
#cmake_install_binpath = "{}/bin".format(cmake_installpath)

# For libboost (set as the default search path of find_package for cmake)
#gpp_include_pathstr, gpp_lib_pathstr = getDefaultIncludeAndLibPathstr()
#boost_preferred_include_dirpath = getPreferredDirpathForTarget("include", "boost", gpp_include_pathstr)
#boost_preferred_lib_dirpath = getPreferredDirpathForTarget("lib", "boost", gpp_lib_pathstr)
boost_preferred_include_dirpath = "/usr/local/include"
boost_preferred_lib_dirpath = "/usr/local/lib"

#print("{} {}".format(boost_preferred_include_dirpath, boost_preferred_lib_dirpath))