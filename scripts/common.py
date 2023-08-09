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

def getPreferredDirpath(infix_name, env_pathstr):
    if env_pathstr == None:
        print("ERROR: env_pathstr is None!")
        sys.exit(1)
    else:
        print("env_pathstr: {}".format(env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in range(len(env_paths)):
        tmp_env_path = env_paths[i]
        if "/usr/local/{}".format(infix_name) in tmp_env_path:
            preferred_dirpath = "/usr/local"
            break
        elif "/usr/{}".format(infix_name) in tmp_env_path:
            preferred_dirpath = "/usr"
            break
    
    if preferred_dirpath == "":
        print("ERROR: both /usr/local/{0} and /usr/{0} are NOT found in {1}!".format(infix_name, env_pathstr))
        sys.exit(1)
    
    return preferred_dirpath

def getDefaultIncludeAndLibPathstr():
    get_gpp_include_pathstr_cmd = "echo | g++ -E -Wp,-v -x c++ - 2>&1 | grep -v '#' | grep -v '^$' | grep -v 'search starts here:' | grep -v 'End of search list.' | grep -v 'ignoring'"
    get_gpp_include_pathstr_subprocess = subprocess.Popen(get_gpp_include_pathstr_cmd, shell=True, capture_output=True)
    if get_gpp_include_pathstr_subprocess.returncode != 0:
        print("ERROR: get_gpp_include_pathstr_subprocess returncode is not 0!")
        sys.exit(1)
    else:
        get_gpp_include_pathstr_outputbytes = get_gpp_include_pathstr_subprocess.stdout
        get_gpp_include_pathstr_outputstr = get_gpp_include_pathstr_outputbytes.decode("utf-8")

        gpp_include_pathstr_elements = get_gpp_include_pathstr_outputstr.split("\n")
        gpp_include_pathstr = ":".join(get_gpp_include_pathstr_elements)
    
    get_gpp_lib_pathstr_cmd = "echo | g++ -E -v -x c++ - 2>&1 | grep 'LIBRARY_PATH'"
    get_gpp_lib_pathstr_subprocess = subprocess.Popen(get_gpp_lib_pathstr_cmd, shell=True, capture_output=True)
    if get_gpp_lib_pathstr_subprocess.returncode != 0:
        print("ERROR: get_gpp_lib_pathstr_subprocess returncode is not 0!")
        sys.exit(1)
    else:
        get_gpp_lib_pathstr_outputbytes = get_gpp_lib_pathstr_subprocess.stdout
        get_gpp_lib_pathstr_outputstr = get_gpp_lib_pathstr_outputbytes.decode("utf-8")

        gpp_lib_pathstr = get_gpp_lib_pathstr_outputstr.split("=")[1]

    return gpp_include_pathstr, gpp_lib_pathstr



preferred_bin_dirpath = getPreferredDirpath("bin", os.getenv("PATH"))
preferred_binpath = "{}/bin".format(preferred_bin_dirpath)

# For python3
python3_installpath = preferred_bin_dirpath
python3_binpath = "{}/bin".format(python3_installpath)

# For gcc/g++
compiler_installpath = "/usr" # Installed by apt
compiler_binpath = "{}/bin".format(compiler_installpath)

# For cmake
#cmake_installpath = "/usr" # Installed by apt
#cmake_binpath = "{}/bin".format(cmake_installpath)

# For libboost
gpp_include_pathstr, gpp_lib_pathstr = getDefaultIncludeAndLibPathstr()
preferred_include_dirpath = getPreferredDirpath("include", gpp_include_pathstr)
preferred_lib_dirpath = getPreferredDirpath("lib", gpp_lib_pathstr)
preferred_includepath = "{}/include".format(preferred_include_dirpath)
preferred_libpath = "{}/lib".format(preferred_lib_dirpath)