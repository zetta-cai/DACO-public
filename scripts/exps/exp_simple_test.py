#!/usr/bin/env python3
# exp_simple_test: an example for usage of exps.utils.prototype and exps.utils.simulator

from .utils.dataset_loader import *
from .utils.prototype import *
from .utils.simulator import *
from .utils.total_statistics_loader import *

exp_simple_test_settings = {
    "clientcnt": 4,
    "edgecnt": 4,
    "keycnt": 1000000,
    "capacity_mb": 3899,
    "cache_name": "gdsf",
    "workload_name": "wikiimage"
}

# For dataset loader
#dataset_loader_instance = DatasetLoader(**exp_simple_test_settings)
#dataset_loader_instance.run()

# For prototype
prototype_instance = Prototype(**exp_simple_test_settings)
prototype_instance.run()

# For simulator
#simulator_instance = Simulator(**exp_simple_test_settings)
#simulator_instance.run()

# For total statistics loader
#total_statistics_loader_instance = TotalStatisticsLoader(**exp_simple_test_settings)
#total_statistics_loader_instance.run()
