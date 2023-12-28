#!/usr/bin/env python3
# logutil: logging utilities to dump normal/prompt/warn/error/emphasize messages

import sys

with_colorama = False

# Log-related variables and functions

def checkColorama():
    global with_colorama, Fore, Back, Style
    if with_colorama == False:
        try:
            from colorama import Fore, Back, Style
            with_colorama = True
        except ImportError:
            with_colorama = False

def dump(scriptname, dumpmsg):
    checkColorama()
    print("{}: {}".format(scriptname, dumpmsg))

def prompt(scriptname, promptmsg):
    checkColorama()
    if with_colorama:
        print(Fore.GREEN + "{}: {}".format(scriptname, promptmsg) + Style.RESET_ALL)
    else:
        print("{}: {}".format(scriptname, promptmsg))

def warn(scriptname, warnmsg):
    checkColorama()
    if with_colorama:
        print(Fore.YELLOW + "[WARN] {}: {}".format(scriptname, warnmsg) + Style.RESET_ALL)
    else:
        print("[WARN] {}: {}".format(scriptname, warnmsg))

def die(scriptname, errmsg):
    checkColorama()
    if with_colorama:
        print(Fore.RED + "[ERROR] {}: {}".format(scriptname, errmsg) + Style.RESET_ALL, file=sys.stderr)
    else:
        print("[ERROR] {}: {}".format(scriptname, errmsg), file=sys.stderr)
    sys.exit(1)

def emphasize(scriptname, emphasize_msg):
    checkColorama()
    if with_colorama:
        print(Fore.MAGENTA + "{}: {}".format(scriptname, emphasize_msg) + Style.RESET_ALL)
    else:
        print("{}: {}".format(scriptname, emphasize_msg))