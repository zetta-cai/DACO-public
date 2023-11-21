#!/usr/bin/env python3

import os
import sys

# Common variables (update TODO parts based on your own testbed)

scriptname = sys.argv[0]
proj_dirname = os.path.dirname(os.path.dirname(os.path.abspath(scriptname)))

#username = os.getenv("USER") # NOTE: USER will be root if sudo is used
username = os.getenv("SUDO_USER")

# TODO: update lib_dirpath for new library installation path if necessary
lib_dirpath = "{}/lib".format(proj_dirname)
if not os.path.exists(lib_dirpath):
    print("{}: Create directory {}...".format(scriptname, lib_dirpath))
    os.mkdir(lib_dirpath)
#else:
#    print("{}: {} exists (directory has been created)".format(scriptname, lib_dirpath))