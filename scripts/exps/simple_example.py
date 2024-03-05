#!/usr/bin/env python3
# simple_example: an example for usage of componenets in exps.utils.

# (1) Import the correpsonding module you need

# from .utils.dataset_loader import *
# from .utils.prototype import *
# from .utils.simulator import *
# from .utils.total_statistics_loader import *

# (2) Define the settings for the experiment

# simple_example_settings = {
#     "clientcnt": 4,
#     "edgecnt": 4,
#     "keycnt": 1000000,
#     "capacity_mb": 1024,
#     "cache_name": "covered",
#     "workload_name": "facebook"
# }

# (3) Choose one util component to run

# For dataset loader
#dataset_loader_instance = DatasetLoader(**simple_example_settings)
#dataset_loader_instance.run()

# For prototype
#prototype_instance = Prototype(**simple_example_settings)
#prototype_instance.run()

# For simulator
#simulator_instance = Simulator(**simple_example_settings)
#simulator_instance.run()

# For total statistics loader
#total_statistics_loader_instance = TotalStatisticsLoader(**simple_example_settings)
#total_statistics_loader_instance.run()