#!/usr/bin/env python3
# exp_simple_test: an example for usage of exps.utils.prototype and exps.utils.simulator

from .utils.prototype import *
from .utils.simulator import *

exp_simple_test_settings = {
    "clientcnt": 3,
    "edgecnt": 3,
    "keycnt": 100000,
    "capacity_mb": 1000,
    "cache_name": "covered"
}

# For prototype
# prototype_instance = Prototype(**exp_simple_test_settings)
# prototype_instance.run()

# For simulator
simulator_instance = Simulator(**exp_simple_test_settings)
simulator_instance.run()