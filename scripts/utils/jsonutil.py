#!/usr/bin/env python3
# JsonUtil: parse config.json to get json content

import json

from .logutil import *

class JsonUtil:
    config_jsonobj_ = None
    with open('config.json', 'r') as f:
        config_jsonobj_ = json.load(f)

    @classmethod
    def getValueForKeystr(cls, scriptname, keystr):
        if cls.config_jsonobj_ is None:
            LogUtil.die(scriptname, "config.json not loaded")

        keystr.strip()
        if keystr not in cls.config_jsonobj_.keys():
            LogUtil.die(scriptname, "Key not found in config.json: " + keystr)
        else:
            return cls.config_jsonobj_[keystr]
    
    @classmethod
    def getFullPathForKeystr(cls, scriptname, keystr, proj_dirname):
        tmp_path_fromjson = cls.getValueForKeystr(scriptname, keystr)
        tmp_path = ""
        if tmp_path_fromjson[0] == "/": # Absolute path
            tmp_path = tmp_path_fromjson
        else: # Relative path
            tmp_path = "{}/{}".format(proj_dirname, tmp_path_fromjson)
        return tmp_path