#!/usr/bin/env python3
# LogUtil: logging utilities to dump normal/prompt/warn/error/emphasize messages

import sys

class LogUtil:
    with_colorama_ = False
    Fore_ = None
    Back_ = None
    Style_ = None

    # Log-related variables and functions

    @classmethod
    def checkColorama(cls):
        if cls.with_colorama_ == False:
            try:
                from colorama import Fore, Back, Style
                cls.Fore_ = Fore
                cls.Back_ = Back
                cls.Style_ = Style
                cls.with_colorama_ = True
            except ImportError:
                cls.with_colorama_ = False

    @classmethod
    def dump(cls, scriptname, dumpmsg):
        cls.checkColorama()
        print("{}: {}".format(scriptname, dumpmsg), flush = True)

    @classmethod
    def prompt(cls, scriptname, promptmsg):
        cls.checkColorama()
        if cls.with_colorama_:
            print(cls.Fore_.GREEN + "{}: {}".format(scriptname, promptmsg) + cls.Style_.RESET_ALL, flush = True)
        else:
            print("{}: {}".format(scriptname, promptmsg), flush = True)

    @classmethod
    def warn(cls, scriptname, warnmsg):
        cls.checkColorama()
        if cls.with_colorama_:
            print(cls.Fore_.YELLOW + "[WARN] {}: {}".format(scriptname, warnmsg) + cls.Style_.RESET_ALL, flush = True)
        else:
            print("[WARN] {}: {}".format(scriptname, warnmsg), flush = True)

    @classmethod
    def die(cls, scriptname, errmsg):
        cls.dieNoExit(scriptname, errmsg)
        sys.exit(1)

    @classmethod
    def dieNoExit(cls, scriptname, errmsg):
        cls.checkColorama()
        if cls.with_colorama_:
            print(cls.Fore_.RED + "[ERROR] {}: {}".format(scriptname, errmsg) + cls.Style_.RESET_ALL, file=sys.stderr, flush = True)
        else:
            print("[ERROR] {}: {}".format(scriptname, errmsg), file=sys.stderr, flush = True)

    @classmethod
    def emphasize(cls, scriptname, emphasize_msg):
        cls.checkColorama()
        if cls.with_colorama_:
            print(cls.Fore_.MAGENTA + "{}: {}".format(scriptname, emphasize_msg) + cls.Style_.RESET_ALL, flush = True)
        else:
            print("{}: {}".format(scriptname, emphasize_msg), flush = True)