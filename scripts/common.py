#!/usr/bin/env python3

import os
import sys

from .utils import *

# Common variables

scriptname = sys.argv[0]
scriptpath = os.path.abspath(scriptname)
proj_dirname = scriptpath[0:scriptpath.find("/scripts")] # [0, index of "/scripts")

username = os.getenv("SUDO_USER") # Get original username if sudo is used
if username is None: # SUDO_USER is None if sudo is not used
    username = os.getenv("USER") # Get username if sudo is not used (NOTE: USER will be root if sudo is used)

# NOTE: update library_path in config.json for new library installation path if necessary
library_dirpath_fromjson = getValueForKeystr(scriptname, "library_dirpath")
lib_dirpath = ""
if library_dirpath_fromjson[0] == "/": # Absolute path
    lib_dirpath = library_dirpath_fromjson
else: # Relative path
    lib_dirpath = "{}/{}".format(proj_dirname, library_dirpath_fromjson)
if not os.path.exists(lib_dirpath):
    prompt(scriptname, "{}: Create directory {}...".format(scriptname, lib_dirpath))
    os.mkdir(lib_dirpath)
#else:
#    dump(scriptname, "{}: {} exists (third-party libarary dirpath has been created)".format(scriptname, lib_dirpath))