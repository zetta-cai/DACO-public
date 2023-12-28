#!/usr/bin/env python3
# launch_prototype: launch prototype over multiple physical machines

from ..common import *

# (1) Launch evaluator

evaluator_finish_initialization_mark = "Evaluator initialized"

evaluator_machine_idx = getValueForKeystr(scriptname, "evaluator_machine_index")
launch_evaluator_cmd = ""
if evaluator_machine_idx == cur_machine_idx:
    launch_evaluator_cmd = "./evaluator"