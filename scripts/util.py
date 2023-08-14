#!/usr/bin/env python3

import sys
from colorama import Fore, Back, Style

def dump(filename, dumpmsg):
    print("{}: {}".format(filename, dumpmsg))

def prompt(filename, promptmsg):
    print(Fore.GREEN + "{}: {}".format(filename, promptmsg) + Style.RESET_ALL)

def warn(filename, warnmsg):
    print(Fore.YELLOW + "[WARN] {}: {}".format(filename, warnmsg) + Style.RESET_ALL)

def die(filename, errmsg):
    print(Fore.RED + "[ERROR] {}: {}".format(filename, errmsg) + Style.RESET_ALL, file=sys.stderr)
    sys.exit(1)

def emphasize(filename, emphasize_msg):
    print(Fore.MAGENTA + "{}: {}".format(filename, emphasize_msg) + Style.RESET_ALL)