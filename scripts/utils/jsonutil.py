#!/usr/bin/env python3

import json

from .logutil import *

config_jsonobj = None

with open('config.json', 'r') as f:
    config_jsonobj = json.load(f)

def getValueForKeystr(scriptname, keystr):
    if config_jsonobj is None:
        die(scriptname, "config.json not loaded")

    keystr.strip()
    if keystr not in config_jsonobj.keys():
        die(scriptname, "Key not found in config.json: " + keystr)
    else:
        return config_jsonobj[keystr]