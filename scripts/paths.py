#!/usr/bin/env python3

import os

filename = sys.argv[0]

lib_dirpath = "lib"
if not os.path.exists(lib_dirpath):
    print("{}: Create directory {}...".format(filename, lib_dirpath))
    os.mkdir(lib_dirpath)
else:
    print("{}: {} exists (directory has been created)".format(filename, lib_dirpath))


def getPreferredDirpath():
    env_pathstr = os.getenv("PATH")

    if env_pathstr == None:
        print("ERROR: $PATH is None!")
        exit()
    else:
        print("$PATH: {}".format(env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in len(env_paths):
        tmp_env_path = env_paths[i]
        if "/usr/local/bin" in tmp_env_path:
            preferred_dirpath = "/usr/local"
            break
        elif "/usr/bin" in tmp_env_path:
            preferred_dirpath = "/usr"
            break
    
    if preferred_dirpath == "":
        print("ERROR: both /usr/local/bin and /usr/bin are NOT found in $PATH!")
        exit()
    
    return preferred_dirpath


preferred_dirpath = getPreferredDirpath()
preferred_binpath = "{}/bin".format(preferred_dirpath)
preferred_libpath = "{}/lib".format(preferred_dirpath)