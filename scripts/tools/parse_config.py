#!/usr/bin/env python3
# parse_config: parse config.json to get value corresponding to keystr (used by Makefile)

from ..common import *

if len(sys.argv) != 2:
    die(scriptname, "Usage: python3 -m scripts.tools.parse_config keystr")

keystr = sys.argv[1]
value = getValueForKeystr(scriptname, keystr)
print(value)