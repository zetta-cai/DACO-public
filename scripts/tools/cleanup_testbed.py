#!/usr/bin/env python3
# cleanup_testbed: kill all related threads launched in testbed.

from ..common import *
from ..exps.utils.exputil import *

related_components = ["./client", "./edge", "./cloud", "./evaluator", "./simulator"]
physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

for tmp_machine_idx in range(len(physical_machines)):
    for tmp_component in related_components:
        ExpUtil.killComponenet_(tmp_machine_idx, tmp_component)