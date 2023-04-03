#!/usr/bin/env python3

import sys
from colorama import Fore, Back, Style

is_common_imported = True

def dump(filename, dumpmsg):
    print("{}: {}".format(filename, dumpmsg))

def prompt(filename, promptmsg):
    print(Fore.GREEN + "{}: {}".format(filename, promptmsg) + Style.RESET_ALL)

def warn(filename, warnmsg):
    print(Fore.BLUE + "{}: {}".format(filename, warnmsg) + Style.RESET_ALL)

def die(filename, errmsg):
    print(Fore.RED + "{}: error {}".format(filename, errmsg) + Style.RESET_ALL, file=sys.stderr)
    sys.exit(1)