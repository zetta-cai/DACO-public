#!/usr/bin/env python3

import os
import sys

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
        # TODO: END HERE
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
        print("ERROR: both /usr/local/{0} and /usr/{0} are NOT found in ${1}!".format(infix_name, env_name))
        sys.exit(1)
    
    return preferred_dirpath


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
preferred_libpath = "{}/lib".format(preferred_bin_dirpath)