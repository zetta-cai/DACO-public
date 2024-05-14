#!/usr/bin/env python3
# characterize_traces: characterize replayed trace files by Zipfian distribution to extract characteristics (including Zipfian constant, key size distribution, and value size distribution) for geo-distributed evaluation.

from .utils.trace_loader import *

# TODO: Check whether each workload has been characterized -> if NOT
# TODO: Get dirpath and filename list by JsonUtil based on the workload name
# TODO: Use TraceLoader to load statistics from trace files for the workload name
# TODO: Curvefit the statistics by Zipfian distribution for the workload name
# TODO; Dump the characteristics for the workload name