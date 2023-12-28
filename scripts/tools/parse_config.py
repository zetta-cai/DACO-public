#!/usr/bin/env python3
# parse_config: parse config.json to get value corresponding to keystr (used by Makefile)

import os

from ..common import *

if len(sys.argv) != 2:
    die(scriptname, "Usage: python3 -m scripts.tools.parse_config keystr")

keystr = sys.argv[1]
value = getValueForKeystr(scriptname, keystr)
print("JSON value: {}".format(value)) # NOTE: used by Makefile to dig value out