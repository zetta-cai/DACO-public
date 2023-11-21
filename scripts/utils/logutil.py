#!/usr/bin/env python3

import sys

with_colorama = False

# Log-related variables and functions

def checkColorama():
    if with_colorama == False:
        try:
            from colorama import Fore, Back, Style
            with_colorama = True
        except ImportError:
            with_colorama = False

def dump(filename, dumpmsg):
    checkColorama()
    print("{}: {}".format(filename, dumpmsg))

def prompt(filename, promptmsg):
    checkColorama()
    if with_colorama:
        print(Fore.GREEN + "{}: {}".format(filename, promptmsg) + Style.RESET_ALL)
    else:
        print("{}: {}".format(filename, promptmsg))

def warn(filename, warnmsg):
    checkColorama()
    if with_colorama:
        print(Fore.YELLOW + "[WARN] {}: {}".format(filename, warnmsg) + Style.RESET_ALL)
    else:
        print("[WARN] {}: {}".format(filename, warnmsg))

def die(filename, errmsg):
    checkColorama()
    if with_colorama:
        print(Fore.RED + "[ERROR] {}: {}".format(filename, errmsg) + Style.RESET_ALL, file=sys.stderr)
    else:
        print("[ERROR] {}: {}".format(filename, errmsg), file=sys.stderr)
    sys.exit(1)

def emphasize(filename, emphasize_msg):
    checkColorama()
    if with_colorama:
        print(Fore.MAGENTA + "{}: {}".format(filename, emphasize_msg) + Style.RESET_ALL)
    else:
        print("{}: {}".format(filename, emphasize_msg))